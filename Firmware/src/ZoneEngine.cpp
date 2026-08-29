#include "ZoneEngine.h"

#include <algorithm>
#include <cmath>

#include "AddWidgetCommand.h"
#include "AddZoneCommand.h"
#include "RemoveWidgetCommand.h"
#include "SetWidgetColourCommand.h"


namespace
{
    constexpr uint32_t NO_LIFESPAN = 0;

    /*
     * Time before activation at which the zone imminent
     * notification is generated.
     */
    constexpr uint32_t ZONE_IMMINENT_SECONDS = 20;
}


ZoneEngine::ZoneEngine(
    const GamePlan& gamePlan,
    DisplayManager& displayManager)
    :
    _pacer(true),
    _gamePlan(gamePlan),
    _displayManager(displayManager),
    _elapsedSeconds(0)
{
    _pacer.SetPace(UpdateRate);
}


/*
 * =============================================================
 * Update
 * =============================================================
 */

void ZoneEngine::Update(
    uint32_t gameTimeSeconds)
{
    if(_pacer.Pace())
    {   
    
        _elapsedSeconds++;
        Serial.print("ZoneEngine T="); Serial.println(_elapsedSeconds);
        UpdateZones(_elapsedSeconds);
        UpdatePickups(_elapsedSeconds);
    }
}


/*
 * =============================================================
 * UpdateZones
 * =============================================================
 */

void ZoneEngine::UpdateZones(
    uint32_t gameTimeSeconds)
{
    const std::vector<Zone>& zones =
        _gamePlan.GetZones();


    int newestActiveZone = -1;

    /*
     * Find the newest currently active zone.
     */
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


    /*
     * Handle zone activation.
     */
    for (size_t i = 0; i < zones.size(); ++i)
    {
        const Zone& zone = zones[i];

        if (!IsZoneActive(
                zone,
                gameTimeSeconds))
        {
            continue;
        }

        if (FindActiveObject(zone.id) == nullptr)
        {
            ActivateZone(
                static_cast<int>(i));
        }
    }


    /*
     * Handle zone expiry.
     *
     * Work backwards because RemoveZoneObjects()
     * modifies _activeObjects.
     */
    for (int i =
             static_cast<int>(_activeObjects.size()) - 1;
         i >= 0;
         --i)
    {
        ActiveGameObject& object =
            _activeObjects[
                static_cast<size_t>(i)];

        if (object.type != WidgetType::Zone)
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
                zones[zoneIndex],
                gameTimeSeconds))
        {
            DeactivateZone(zoneIndex);
        }
    }


    /*
     * The most recently active zone is green.
     *
     * If another zone is still active, it is yellow.
     */
    if (newestActiveZone >= 0)
    {
        for (size_t i = 0; i < zones.size(); ++i)
        {
            ActiveGameObject* object =
                FindActiveObject(zones[i].id);

            if (object == nullptr)
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
        if (_imminentZoneIndex != imminentZone)
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


    _currentZoneIndex =
        newestActiveZone;
}


/*
 * =============================================================
 * UpdatePickups
 * =============================================================
 */

void ZoneEngine::UpdatePickups(
    uint32_t gameTimeSeconds)
{
    const std::vector<Zone>& zones =
        _gamePlan.GetZones();

    /*
     * Add pickups that have become active.
     */
    for (const Zone& zone : zones)
    {
        for (const auto& pickupEntry : zone.pickups)
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

            if (FindActiveObject(pickup.id) ==
                nullptr)
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
             static_cast<int>(_activeObjects.size()) - 1;
         i >= 0;
         --i)
    {
        const ActiveGameObject& object =
            _activeObjects[
                static_cast<size_t>(i)];

        if (object.type != WidgetType::Pickup)
        {
            continue;
        }

        const Pickup* pickup = nullptr;
        const Zone* owningZone = nullptr;

        for (const Zone& zone : zones)
        {
            for (const auto& pickupEntry : zone.pickups)
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


/*
 * =============================================================
 * IsZoneActive
 * =============================================================
 */

bool ZoneEngine::IsZoneActive(
    const Zone& zone,
    uint32_t gameTimeSeconds) const
{
    if (gameTimeSeconds < zone.startTime)
    {
        return false;
    }

    if (zone.lifespanSeconds == NO_LIFESPAN)
    {
        return true;
    }

    const uint32_t expiry =
        zone.startTime +
        zone.lifespanSeconds;

    return gameTimeSeconds < expiry;
}


/*
 * =============================================================
 * IsPickupActive
 * =============================================================
 */

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


/*
 * =============================================================
 * ActivateZone
 * =============================================================
 */

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
            static_cast<size_t>(zoneIndex)];

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

    object.zoneId =
        zone.id;

    object.claimed =
        false;

    _activeObjects.push_back(
        object);
}


/*
 * =============================================================
 * DeactivateZone
 * =============================================================
 */

void ZoneEngine::DeactivateZone(
    int zoneIndex)
{
    const std::vector<Zone>& zones =
        _gamePlan.GetZones();
    Serial.printf("Deactivating zone %d\n", zoneIndex);
    if (zoneIndex < 0 ||
        zoneIndex >=
            static_cast<int>(zones.size()))
    {
        return;
    }

    const Zone& zone =
        zones[
            static_cast<size_t>(zoneIndex)];

    NotifyZoneExpired(
        zone,
        zoneIndex);

    /*
     * Remove all pickups belonging to the zone
     * as well as the zone itself.
     */
    RemoveZoneObjects(
        zone.id);
}


/*
 * =============================================================
 * ActivatePickup
 * =============================================================
 */

void ZoneEngine::ActivatePickup(
    const Zone& zone,
    const Pickup& pickup)
{
    const char* filename = nullptr;
    Serial.printf("Activating pickup  %d\n", pickup.id);
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

    object.zoneId =
        zone.id;

    object.claimed =
        false;

    _activeObjects.push_back(
        object);

    NotifyPickupActivated(
        pickup.id);
}


/*
 * =============================================================
 * DeactivatePickup
 * =============================================================
 */

void ZoneEngine::DeactivatePickup(
    uint32_t pickupId)
{
    ActiveGameObject* object =
        FindActiveObject(pickupId);
    Serial.printf("Deactivating pickup  %d\n", pickupId);
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

    _activeObjects.erase(
        std::remove_if(
            _activeObjects.begin(),
            _activeObjects.end(),
            [pickupId](
                const ActiveGameObject& object)
            {
                return object.id == pickupId;
            }),
        _activeObjects.end());
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
                return object.id == objectId;
            }),
        _activeObjects.end());
}

/*
 * =============================================================
 * RemoveZoneObjects
 * =============================================================
 */
void ZoneEngine::RemoveZoneObjects(
    uint32_t zoneId)
{
    const size_t zoneIndex =
        zoneId & ~GamePlan::ZoneWidgetIdBase;

    const auto& zones =
        _gamePlan.GetZones();

    Serial.printf("Removeing zone objects %d\n", zoneId);
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

    RemoveActiveObject(zone.id);

    for (const auto& pickupEntry : zone.pickups)
    {
        const Pickup& pickup =
            pickupEntry.second;

        _displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new RemoveWidgetCommand(
                    pickup.id)));

        RemoveActiveObject(pickup.id);
    }
}


/*
 * =============================================================
 * FindActiveObject
 * =============================================================
 */

ZoneEngine::ActiveGameObject*
ZoneEngine::FindActiveObject(
    uint32_t objectId)
{
    for (ActiveGameObject& object :
         _activeObjects)
    {
        if (object.id == objectId)
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
        if (object.id == objectId)
        {
            return &object;
        }
    }

    return nullptr;
}


/*
 * =============================================================
 * GetPickupsWithin
 * =============================================================
 */

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
            WidgetType::Pickup)
        {
            continue;
        }

        if (object.claimed)
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

        /*
         * Recover the Pickup type from GamePlan.
         */
        for (const Zone& zone :
             _gamePlan.GetZones())
        {
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
                    break;
                }
            }

            if (resultCount > 0 &&
                results[resultCount - 1].id ==
                    object.id)
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


/*
 * =============================================================
 * ClaimPickup
 * =============================================================
 */

bool ZoneEngine::ClaimPickup(
    uint32_t pickupId)
{
    ActiveGameObject* object =
        FindActiveObject(pickupId);

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

    _activeObjects.erase(
        std::remove_if(
            _activeObjects.begin(),
            _activeObjects.end(),
            [pickupId](
                const ActiveGameObject& object)
            {
                return object.id == pickupId;
            }),
        _activeObjects.end());

    return true;
}


/*
 * =============================================================
 * Listener management
 * =============================================================
 */

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


/*
 * =============================================================
 * Event notification
 * =============================================================
 */

void ZoneEngine::NotifyPickupActivated(
    uint32_t pickupId)
{
    Serial.printf("Pickup activated: %d\n", pickupId);
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
    Serial.printf("Pickup deactivated: %d\n", pickupId);
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
    Serial.printf("Zone Imminent: %d\n", zone.id);
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
    Serial.printf("Zone expired: %d\n", zone.id);
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

void ZoneEngine::Reset()
{
    _elapsedSeconds = 0;
}