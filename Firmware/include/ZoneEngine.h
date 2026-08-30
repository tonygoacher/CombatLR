#pragma once

#include <Arduino.h>
#include <deque>
#include <vector>

#include "GamePlan.h"
#include "DisplayManager.h"
#include "IPickupEvent.h"
#include "IZoneEvent.h"
#include "IWidget.h"
#include "Pacer.h"

class ZoneEngine
{
public:

    struct PickupInfo
    {
        uint32_t id;
        GameCoordinate position;
        PickupTypes type;
    };

    ZoneEngine(
        const GamePlan& gamePlan,
        DisplayManager& displayManager);

    void Update(
        uint32_t gameTimeSeconds);

    size_t GetPickupsWithin(
        const GameCoordinate& playerPosition,
        float radiusMetres,
        PickupInfo* results,
        size_t maxResults) const;

    bool ClaimPickup(
        uint32_t pickupId);

    void AddPickupEventListener(
        IPickupEvent* listener);

    void RemovePickupEventListener(
        IPickupEvent* listener);

    void AddZoneEventListener(
        IZoneEvent* listener);

    void RemoveZoneEventListener(
        IZoneEvent* listener);

private:

    static const int UpdateRate = 1000;
    static constexpr uint32_t ZoneTransitionUpdateMs = 100;

    struct ActiveGameObject
    {
        uint32_t id;
        GameCoordinate position;
        WidgetType type;
        bool claimed;
    };

    struct ZoneTransition
    {
        int fromZoneIndex;
        int toZoneIndex;
    };

    Pacer _pacer;

    uint32_t _lastGameTimeSecond = UINT32_MAX;

    const GamePlan& _gamePlan;
    DisplayManager& _displayManager;

    std::vector<ActiveGameObject> _activeObjects;

    std::deque<IPickupEvent*> _pickupEventListeners;
    std::deque<IZoneEvent*> _zoneEventListeners;

    int _currentZoneIndex = -1;
    int _imminentZoneIndex = -1;

    bool _zoneTransitionActive = false;
    ZoneTransition _zoneTransition{ -1, -1 };

    Pacer _transitionPacer;

    uint32_t _transitionStartMillis = 0;
    uint32_t _transitionDurationMillis = 0;

    static const uint8_t RedshiftMin = 7;

    uint8_t _redshift = RedshiftMin;

    void UpdateZones(
        uint32_t gameTimeSeconds);

    void UpdatePickups(
        uint32_t gameTimeSeconds);

    void UpdateZoneTransition();

    bool IsZoneActive(
        const Zone& zone,
        uint32_t gameTimeSeconds) const;

    bool IsPickupActive(
        const Pickup& pickup,
        uint32_t gameTimeSeconds) const;

    void ActivateZone(
        int zoneIndex);

    void DeactivateZone(
        int zoneIndex);

    void ActivatePickup(
        const Zone& zone,
        const Pickup& pickup);

    void DeactivatePickup(
        uint32_t pickupId);

    void RemoveZoneObjects(
        uint32_t zoneId);

    void RemoveActiveObject(
        uint32_t objectId);

    ActiveGameObject* FindActiveObject(
        uint32_t objectId);

    const ActiveGameObject* FindActiveObject(
        uint32_t objectId) const;

    void StartZoneTransition(
        int fromZoneIndex,
        int toZoneIndex);

    void CompleteZoneTransition();

    void NotifyPickupActivated(
        uint32_t pickupId);

    void NotifyPickupDeactivated(
        uint32_t pickupId);

    void NotifyZoneImminent(
        const Zone& zone,
        int zoneIndex,
        uint32_t timeToActivation);

    void NotifyZoneExpired(
        const Zone& zone,
        int zoneIndex);

    void NotifyZoneTransitionComplete(
        int zoneIndex);

    void Reset();
};
