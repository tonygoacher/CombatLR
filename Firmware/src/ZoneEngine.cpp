#include "ZoneEngine.h"

#include <algorithm>
#include <cmath>

#include "AddWidgetCommand.h"
#include "AddZoneCommand.h"
#include "RemoveWidgetCommand.h"
#include "SetWidgetColourCommand.h"
#include "SetZoneGeometryCommand.h"

namespace
{
    constexpr uint32_t NO_LIFESPAN = 0;
    constexpr uint32_t ZONE_IMMINENT_SECONDS = 20;
    constexpr uint32_t ZONE_TRANSITION_UPDATE_MS = 100;
    constexpr uint32_t ZONE_TRANSITION_FLASH_MS = 250;
}

ZoneEngine::ZoneEngine(
    const GamePlan& gamePlan,
    DisplayManager& displayManager)
    :
    _pacer(true),
    _gamePlan(gamePlan),
    _displayManager(displayManager),
    _transitionPacer(true)
{
    _pacer.SetPace(UpdateRate);
    _transitionPacer.SetPace(
        ZONE_TRANSITION_UPDATE_MS);
}

void ZoneEngine::Update(
    uint32_t gameTimeSeconds)
{
    /*
     * Game state is evaluated only when the supplied game
     * time moves to a new second.
     */
    if (gameTimeSeconds != _lastGameTimeSecond)
    {
        Serial.printf("ZoneEngine T:%d\n", gameTimeSeconds);
        _lastGameTimeSecond =
            gameTimeSeconds;

        UpdateZones(
            gameTimeSeconds);

        UpdatePickups(
            gameTimeSeconds);
    }

    /*
     * Visual transition movement is independently paced.
     */
    if (_zoneTransitionActive &&
        _transitionPacer.Pace())
    {
        UpdateZoneTransition();
    }
}

void ZoneEngine::UpdateZones(
    uint32_t gameTimeSeconds)
{
    const std::vector<Zone>& zones =
        _gamePlan.GetZones();

    /*
     * Find the newest zone whose start time has been
     * reached. We deliberately do not use IsZoneActive()
     * here because a zone which is currently in its
     * transition/overlap period is still the current zone.
     */
    int newestStartedZone = -1;

    for (size_t i = 0; i < zones.size(); ++i)
    {
        if (gameTimeSeconds >= zones[i].startTime)
        {
            newestStartedZone =
                static_cast<int>(i);
        }
    }

    /*
     * Initial activation.
     */
    if (_currentZoneIndex < 0 &&
        newestStartedZone >= 0)
    {
        ActivateZone(
            newestStartedZone);

        _currentZoneIndex =
            newestStartedZone;
    }

    /*
     * Start the visual transition before the new zone's
     * start time. The duration is simply the difference
     * between the two GamePlan start times.
     *
     * This is deliberately started by the imminent-zone
     * mechanism, so the transition can last for the whole
     * overlap period rather than merely the final 20s.
     */
    if (!_zoneTransitionActive &&
        _currentZoneIndex >= 0 &&
        _currentZoneIndex + 1 <
            static_cast<int>(zones.size()))
    {
        const Zone& fromZone =
            zones[
                static_cast<size_t>(
                    _currentZoneIndex)];

        const Zone& toZone =
            zones[
                static_cast<size_t>(
                    _currentZoneIndex + 1)];

        const uint32_t fromEnd =
            fromZone.startTime +
            fromZone.lifespanSeconds;

        if (fromZone.lifespanSeconds != NO_LIFESPAN &&
            toZone.startTime < fromEnd)
        {
            const uint32_t overlapSeconds =
                fromEnd - toZone.startTime;

            const uint32_t transitionStartTime =
                toZone.startTime - overlapSeconds;

            if (gameTimeSeconds >= transitionStartTime &&
                gameTimeSeconds < toZone.startTime)
            {
                StartZoneTransition(
                    _currentZoneIndex,
                    _currentZoneIndex + 1);
            }
        }
    }

    /*
     * The new zone takes ownership exactly at its start
     * time. The renderer does not perform this operation.
     */
    if (_zoneTransitionActive)
    {
        const Zone& toZone =
            zones[
                static_cast<size_t>(
                    _zoneTransition.toZoneIndex)];

        if (gameTimeSeconds >= toZone.startTime)
        {
            CompleteZoneTransition();
        }
    }

    /*
     * Handle ordinary expiry for zones which are not
     * involved in the transition hand-over.
     */
    for (int i =
             static_cast<int>(
                 _activeObjects.size()) - 1;
         i >= 0;
         --i)
    {
        ActiveGameObject& object =
            _activeObjects[
                static_cast<size_t>(i)];

        if (object.type !=
            WidgetType::Zone)
        {
            continue;
        }

        if (_zoneTransitionActive &&
            object.id ==
                zones[
                    static_cast<size_t>(
                        _zoneTransition.fromZoneIndex)].id)
        {
            continue;
        }

        int zoneIndex = -1;

        for (size_t z = 0; z < zones.size(); ++z)
        {
            if (zones[z].id == object.id)
            {
                zoneIndex =
                    static_cast<int>(z);
                break;
            }
        }

        if (zoneIndex < 0)
        {
            continue;
        }

        if (!IsZoneActive(
                zones[
                    static_cast<size_t>(
                        zoneIndex)],
                gameTimeSeconds))
        {
            DeactivateZone(
                zoneIndex);

            if (zoneIndex ==
                _currentZoneIndex)
            {
                _currentZoneIndex = -1;
            }
        }
    }

    /*
     * The current zone is green. Older overlapping zones
     * are yellow. The moving zone's flashing is handled
     * separately by UpdateZoneTransition().
     */
    int newestActiveZone = -1;

    for (size_t i = 0; i < zones.size(); ++i)
    {
        if (IsZoneActive(
                zones[i],
                gameTimeSeconds))
        {
            newestActiveZone =
                static_cast<int>(i);
        }
    }

    if (newestActiveZone >= 0)
    {
        for (size_t i = 0; i < zones.size(); ++i)
        {
            ActiveGameObject* object =
                FindActiveObject(
                    zones[i].id);

            if (object == nullptr)
            {
                continue;
            }

            if (_zoneTransitionActive &&
                static_cast<int>(i) ==
                    _zoneTransition.fromZoneIndex)
            {
                continue;
            }

            const uint32_t colour =
                (static_cast<int>(i) ==
                 newestActiveZone)
                    ? TFT_GREEN
                    : TFT_YELLOW;

            _displayManager.Send(
                std::unique_ptr<IDisplayCommand>(
                    new SetWidgetColourCommand(
                        object->id,
                        colour)));
        }
    }

    /*
     * Zone imminent notification.
     *
     * This is independent of transition start. It remains
     * a one-shot notification for the configured threshold.
     */
    int imminentZone = -1;

    for (size_t i = 0; i < zones.size(); ++i)
    {
        const Zone& zone = zones[i];

        if (gameTimeSeconds >= zone.startTime)
        {
            continue;
        }

        const uint32_t timeToActivation =
            zone.startTime -
            gameTimeSeconds;

        if (timeToActivation <=
            ZONE_IMMINENT_SECONDS)
        {
            imminentZone =
                static_cast<int>(i);
            break;
        }
    }

    if (imminentZone >= 0)
    {
        if (_imminentZoneIndex !=
            imminentZone)
        {
            const Zone& zone =
                zones[
                    static_cast<size_t>(
                        imminentZone)];

            NotifyZoneImminent(
                zone,
                imminentZone,
                zone.startTime -
                    gameTimeSeconds);

            _imminentZoneIndex =
                imminentZone;
        }
    }
    else
    {
        _imminentZoneIndex = -1;
    }
}

void ZoneEngine::UpdatePickups(
    uint32_t gameTimeSeconds)
{
    const std::vector<Zone>& zones =
        _gamePlan.GetZones();

    /*
     * Only pickups belonging to the current zone are
     * activated. During transition the old zone continues
     * to own its pickups.
     */
    if (_currentZoneIndex >= 0 &&
        !_zoneTransitionActive &&
        _currentZoneIndex <
            static_cast<int>(zones.size()))
    {
        const Zone& zone =
            zones[
                static_cast<size_t>(
                    _currentZoneIndex)];

        for (const auto& pickupEntry :
             zone.pickups)
        {
            const Pickup& pickup =
                pickupEntry.second;

            if (!IsZoneActive(
                    zone,
                    gameTimeSeconds))
            {
                continue;
            }

            if (!IsPickupActive(
                    pickup,
                    gameTimeSeconds))
            {
                continue;
            }

            if (pickup.id == 0)
            {
                continue;
            }

            if (FindActiveObject(
                    pickup.id) == nullptr)
            {
                ActivatePickup(
                    zone,
                    pickup);
            }
        }
    }

    /*
     * Remove expired pickups.
     */
    for (int i =
             static_cast<int>(
                 _activeObjects.size()) - 1;
         i >= 0;
         --i)
    {
        const ActiveGameObject& object =
            _activeObjects[
                static_cast<size_t>(i)];

        if (object.type !=
            WidgetType::Pickup)
        {
            continue;
        }

        const Pickup* pickup = nullptr;
        const Zone* owningZone = nullptr;

        for (const Zone& zone :
             zones)
        {
            for (const auto& pickupEntry :
                 zone.pickups)
            {
                if (pickupEntry.second.id ==
                    object.id)
                {
                    pickup =
                        &pickupEntry.second;
                    owningZone =
                        &zone;
                    break;
                }
            }

            if (pickup != nullptr)
            {
                break;
            }
        }

        if (pickup == nullptr ||
            owningZone == nullptr)
        {
            continue;
        }

        if (!IsZoneActive(
                *owningZone,
                gameTimeSeconds) ||
            !IsPickupActive(
                *pickup,
                gameTimeSeconds))
        {
            DeactivatePickup(
                pickup->id);
        }
    }
}

void ZoneEngine::UpdateZoneTransition()
{
    if (!_zoneTransitionActive)
    {
        return;
    }

    const std::vector<Zone>& zones =
        _gamePlan.GetZones();

    if (_zoneTransition.fromZoneIndex < 0 ||
        _zoneTransition.toZoneIndex < 0 ||
        _zoneTransition.fromZoneIndex >=
            static_cast<int>(zones.size()) ||
        _zoneTransition.toZoneIndex >=
            static_cast<int>(zones.size()))
    {
        _zoneTransitionActive = false;
        return;
    }

    const Zone& fromZone =
        zones[
            static_cast<size_t>(
                _zoneTransition.fromZoneIndex)];

    const Zone& toZone =
        zones[
            static_cast<size_t>(
                _zoneTransition.toZoneIndex)];

    const uint32_t elapsedMillis =
        millis() -
        _transitionStartMillis;

    float progress = 1.0f;

    if (_transitionDurationMillis != 0)
    {
        progress =
            static_cast<float>(
                elapsedMillis) /
            static_cast<float>(
                _transitionDurationMillis);

        if (progress > 1.0f)
        {
            progress = 1.0f;
        }
    }

    const double east =
        fromZone.eastMetres +
        (toZone.eastMetres -
         fromZone.eastMetres) *
        progress;

    const double north =
        fromZone.northMetres +
        (toZone.northMetres -
         fromZone.northMetres) *
        progress;

    const double radius =
        fromZone.radiusMetres +
        (toZone.radiusMetres -
         fromZone.radiusMetres) *
        progress;

    _displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new SetZoneGeometryCommand(
                fromZone.id,
                GameCoordinate(
                    east,
                    north),
                radius)));

    /*
     * Flash while moving.
     */
    const uint32_t flashPhase =
        (elapsedMillis /
         ZONE_TRANSITION_FLASH_MS) & 1U;

    const uint32_t colour =
        flashPhase
            ? TFT_YELLOW
            : TFT_GREEN;

    _displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new SetWidgetColourCommand(
                fromZone.id,
                colour)));
}

void ZoneEngine::StartZoneTransition(
    int fromZoneIndex,
    int toZoneIndex)
{
    const std::vector<Zone>& zones =
        _gamePlan.GetZones();

    if (fromZoneIndex < 0 ||
        toZoneIndex < 0 ||
        fromZoneIndex >=
            static_cast<int>(zones.size()) ||
        toZoneIndex >=
            static_cast<int>(zones.size()) ||
        toZoneIndex <= fromZoneIndex)
    {
        return;
    }

    const Zone& fromZone =
        zones[
            static_cast<size_t>(
                fromZoneIndex)];

    const Zone& toZone =
        zones[
            static_cast<size_t>(
                toZoneIndex)];

    if (toZone.startTime <=
        fromZone.startTime)
    {
        return;
    }

    const uint32_t fromEnd =
        fromZone.startTime +
        fromZone.lifespanSeconds;

    if (fromZone.lifespanSeconds == NO_LIFESPAN ||
        toZone.startTime >= fromEnd)
    {
        return;
    }

    _zoneTransition.fromZoneIndex =
        fromZoneIndex;

    _zoneTransition.toZoneIndex =
        toZoneIndex;

    _zoneTransitionActive =
        true;

    const uint32_t overlapSeconds =
        fromEnd - toZone.startTime;

    _transitionDurationMillis =
        overlapSeconds * 1000UL;

    /*
     * The transition starts at:
     *
     *     next zone start - overlap
     *
     * and finishes exactly when the next zone starts.
     */
    _transitionStartMillis =
        millis();

    _transitionPacer.PacerReset();

    /*
     * Start at the old zone's exact geometry.
     */
    _displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new SetZoneGeometryCommand(
                fromZone.id,
                GameCoordinate(
                    fromZone.eastMetres,
                    fromZone.northMetres),
                fromZone.radiusMetres)));

    _displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new SetWidgetColourCommand(
                fromZone.id,
                TFT_YELLOW)));
}

void ZoneEngine::CompleteZoneTransition()
{
    if (!_zoneTransitionActive)
    {
        return;
    }

    const std::vector<Zone>& zones =
        _gamePlan.GetZones();

    const int oldZoneIndex =
        _zoneTransition.fromZoneIndex;

    const int newZoneIndex =
        _zoneTransition.toZoneIndex;

    if (oldZoneIndex < 0 ||
        newZoneIndex < 0 ||
        oldZoneIndex >=
            static_cast<int>(zones.size()) ||
        newZoneIndex >=
            static_cast<int>(zones.size()))
    {
        _zoneTransitionActive = false;
        return;
    }

    const Zone& oldZone =
        zones[
            static_cast<size_t>(
                oldZoneIndex)];

    RemoveZoneObjects(
        oldZone.id);

    _zoneTransitionActive =
        false;

    ActivateZone(
        newZoneIndex);

    _currentZoneIndex =
        newZoneIndex;

    NotifyZoneTransitionComplete(
        newZoneIndex);
}

bool ZoneEngine::IsZoneActive(
    const Zone& zone,
    uint32_t gameTimeSeconds) const
{
    if (gameTimeSeconds <
        zone.startTime)
    {
        return false;
    }

    if (zone.lifespanSeconds ==
        NO_LIFESPAN)
    {
        return true;
    }

    const uint32_t expiry =
        zone.startTime +
        zone.lifespanSeconds;

    return gameTimeSeconds < expiry;
}

bool ZoneEngine::IsPickupActive(
    const Pickup& pickup,
    uint32_t gameTimeSeconds) const
{
    if (gameTimeSeconds <
        pickup.spawnTimeSeconds)
    {
        return false;
    }

    if (pickup.lifespanSeconds ==
        NO_LIFESPAN)
    {
        return true;
    }

    const uint32_t expiry =
        pickup.spawnTimeSeconds +
        pickup.lifespanSeconds;

    return gameTimeSeconds < expiry;
}

void ZoneEngine::ActivateZone(
    int zoneIndex)
{
    const std::vector<Zone>& zones =
        _gamePlan.GetZones();

    if (zoneIndex < 0 ||
        zoneIndex >=
            static_cast<int>(zones.size()))
    {
        return;
    }

    const Zone& zone =
        zones[
            static_cast<size_t>(
                zoneIndex)];

    AddZoneCommand* command =
        new AddZoneCommand(
            zone.id,
            GameCoordinate(
                zone.eastMetres,
                zone.northMetres),
            zone.radiusMetres,
            TFT_GREEN);

    if (!_displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                command)))
    {
        delete command;
        return;
    }

    ActiveGameObject object;

    object.id =
        zone.id;

    object.position =
        GameCoordinate(
            zone.eastMetres,
            zone.northMetres);

    object.type =
        WidgetType::Zone;

    object.claimed =
        false;

    _activeObjects.push_back(
        object);
}

void ZoneEngine::DeactivateZone(
    int zoneIndex)
{
    const std::vector<Zone>& zones =
        _gamePlan.GetZones();

    if (zoneIndex < 0 ||
        zoneIndex >=
            static_cast<int>(zones.size()))
    {
        return;
    }

    const Zone& zone =
        zones[
            static_cast<size_t>(
                zoneIndex)];

    NotifyZoneExpired(
        zone,
        zoneIndex);

    RemoveZoneObjects(
        zone.id);
}

void ZoneEngine::ActivatePickup(
    const Zone& zone,
    const Pickup& pickup)
{
    (void)zone;

    const char* filename = nullptr;

    switch (pickup.type)
    {
        case PickupTypes::Health:
            filename = "/health.bmp";
            break;

        case PickupTypes::Ammo:
            filename = "/reload.bmp";
            break;

        case PickupTypes::Bonus:
            filename = "/weapon.bmp";
            break;

        default:
            return;
    }

    AddWidgetCommand* command =
        new AddWidgetCommand(
            pickup.id,
            WidgetType::Pickup,
            pickup.GetPosition(),
            filename);

    if (!_displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                command)))
    {
        delete command;
        return;
    }

    ActiveGameObject object;

    object.id =
        pickup.id;

    object.position =
        pickup.GetPosition();

    object.type =
        WidgetType::Pickup;

    object.claimed =
        false;

    _activeObjects.push_back(
        object);

    NotifyPickupActivated(
        pickup.id);
}

void ZoneEngine::DeactivatePickup(
    uint32_t pickupId)
{
    ActiveGameObject* object =
        FindActiveObject(
            pickupId);

    if (object == nullptr)
    {
        return;
    }

    _displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new RemoveWidgetCommand(
                pickupId)));

    NotifyPickupDeactivated(
        pickupId);

    RemoveActiveObject(
        pickupId);
}

void ZoneEngine::RemoveActiveObject(
    uint32_t objectId)
{
    _activeObjects.erase(
        std::remove_if(
            _activeObjects.begin(),
            _activeObjects.end(),
            [objectId](
                const ActiveGameObject& object)
            {
                return object.id ==
                    objectId;
            }),
        _activeObjects.end());
}

void ZoneEngine::RemoveZoneObjects(
    uint32_t zoneId)
{
    const size_t zoneIndex =
        zoneId &
        ~GamePlan::ZoneWidgetIdBase;

    const std::vector<Zone>& zones =
        _gamePlan.GetZones();

    if (zoneIndex >= zones.size())
    {
        return;
    }

    const Zone& zone =
        zones[zoneIndex];

    _displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new RemoveWidgetCommand(
                zone.id)));

    RemoveActiveObject(
        zone.id);

    for (const auto& pickupEntry :
         zone.pickups)
    {
        const Pickup& pickup =
            pickupEntry.second;

        _displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new RemoveWidgetCommand(
                    pickup.id)));

        RemoveActiveObject(
            pickup.id);
    }
}

ZoneEngine::ActiveGameObject*
ZoneEngine::FindActiveObject(
    uint32_t objectId)
{
    for (ActiveGameObject& object :
         _activeObjects)
    {
        if (object.id ==
            objectId)
        {
            return &object;
        }
    }

    return nullptr;
}

const ZoneEngine::ActiveGameObject*
ZoneEngine::FindActiveObject(
    uint32_t objectId) const
{
    for (const ActiveGameObject& object :
         _activeObjects)
    {
        if (object.id ==
            objectId)
        {
            return &object;
        }
    }

    return nullptr;
}

size_t ZoneEngine::GetPickupsWithin(
    const GameCoordinate& playerPosition,
    float radiusMetres,
    PickupInfo* results,
    size_t maxResults) const
{
    if (results == nullptr ||
        maxResults == 0 ||
        radiusMetres < 0.0f)
    {
        return 0;
    }

    const float radiusSquared =
        radiusMetres *
        radiusMetres;

    size_t resultCount = 0;

    for (const ActiveGameObject& object :
         _activeObjects)
    {
        if (object.type !=
            WidgetType::Pickup ||
            object.claimed)
        {
            continue;
        }

        const float east =
            static_cast<float>(
                object.position.east -
                playerPosition.east);

        const float north =
            static_cast<float>(
                object.position.north -
                playerPosition.north);

        const float distanceSquared =
            east * east +
            north * north;

        if (distanceSquared >
            radiusSquared)
        {
            continue;
        }

        for (const Zone& zone :
             _gamePlan.GetZones())
        {
            bool found = false;

            for (const auto& entry :
                 zone.pickups)
            {
                if (entry.second.id ==
                    object.id)
                {
                    results[resultCount].id =
                        object.id;

                    results[resultCount].position =
                        object.position;

                    results[resultCount].type =
                        entry.second.type;

                    ++resultCount;

                    found = true;
                    break;
                }
            }

            if (found)
            {
                break;
            }
        }

        if (resultCount >= maxResults)
        {
            break;
        }
    }

    return resultCount;
}

bool ZoneEngine::ClaimPickup(
    uint32_t pickupId)
{
    ActiveGameObject* object =
        FindActiveObject(
            pickupId);

    if (object == nullptr ||
        object->type !=
            WidgetType::Pickup ||
        object->claimed)
    {
        return false;
    }

    object->claimed = true;

    _displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new RemoveWidgetCommand(
                pickupId)));

    RemoveActiveObject(
        pickupId);

    return true;
}

void ZoneEngine::AddPickupEventListener(
    IPickupEvent* listener)
{
    if (listener == nullptr)
    {
        return;
    }

    if (std::find(
            _pickupEventListeners.begin(),
            _pickupEventListeners.end(),
            listener) ==
        _pickupEventListeners.end())
    {
        _pickupEventListeners.push_back(
            listener);
    }
}

void ZoneEngine::RemovePickupEventListener(
    IPickupEvent* listener)
{
    _pickupEventListeners.erase(
        std::remove(
            _pickupEventListeners.begin(),
            _pickupEventListeners.end(),
            listener),
        _pickupEventListeners.end());
}

void ZoneEngine::AddZoneEventListener(
    IZoneEvent* listener)
{
    if (listener == nullptr)
    {
        return;
    }

    if (std::find(
            _zoneEventListeners.begin(),
            _zoneEventListeners.end(),
            listener) ==
        _zoneEventListeners.end())
    {
        _zoneEventListeners.push_back(
            listener);
    }
}

void ZoneEngine::RemoveZoneEventListener(
    IZoneEvent* listener)
{
    _zoneEventListeners.erase(
        std::remove(
            _zoneEventListeners.begin(),
            _zoneEventListeners.end(),
            listener),
        _zoneEventListeners.end());
}

void ZoneEngine::NotifyPickupActivated(
    uint32_t pickupId)
{
    for (IPickupEvent* listener :
         _pickupEventListeners)
    {
        if (listener != nullptr)
        {
            listener->PickupActivated(
                pickupId);
        }
    }
}

void ZoneEngine::NotifyPickupDeactivated(
    uint32_t pickupId)
{
    for (IPickupEvent* listener :
         _pickupEventListeners)
    {
        if (listener != nullptr)
        {
            listener->PickupDeactivated(
                pickupId);
        }
    }
}

void ZoneEngine::NotifyZoneImminent(
    const Zone& zone,
    int zoneIndex,
    uint32_t timeToActivation)
{
    const GameCoordinate position(
        zone.eastMetres,
        zone.northMetres);

    for (IZoneEvent* listener :
         _zoneEventListeners)
    {
        if (listener != nullptr)
        {
            listener->ZoneImminent(
                position,
                zoneIndex,
                timeToActivation);
        }
    }
}

void ZoneEngine::NotifyZoneExpired(
    const Zone& zone,
    int zoneIndex)
{
    const GameCoordinate position(
        zone.eastMetres,
        zone.northMetres);

    for (IZoneEvent* listener :
         _zoneEventListeners)
    {
        if (listener != nullptr)
        {
            listener->ZoneExpired(
                position,
                zoneIndex);
        }
    }
}

void ZoneEngine::NotifyZoneTransitionComplete(
    int zoneIndex)
{
    for (IZoneEvent* listener :
         _zoneEventListeners)
    {
        if (listener != nullptr)
        {
            listener->ZoneTransitionComplete(
                zoneIndex);
        }
    }
}

void ZoneEngine::Reset()
{
    _lastGameTimeSecond =
        UINT32_MAX;

    _currentZoneIndex =
        -1;

    _imminentZoneIndex =
        -1;

    _zoneTransitionActive =
        false;

    _zoneTransition =
        { -1, -1 };

    _transitionStartMillis =
        0;

    _transitionDurationMillis =
        0;

    _transitionPacer.PacerReset();

    _activeObjects.clear();
}
