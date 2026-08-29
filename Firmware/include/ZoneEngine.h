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
    static const int UpdateRate = 1000; // Update every 1000ms
    struct ActiveGameObject
    {
        uint32_t id;
        GameCoordinate position;
        WidgetType type;
        uint32_t zoneId;
        bool claimed;
    };

    Pacer _pacer;
    uint32_t _elapsedSeconds;

    const GamePlan& _gamePlan;
    DisplayManager& _displayManager;

    std::vector<ActiveGameObject> _activeObjects;

    std::deque<IPickupEvent*> _pickupEventListeners;
    std::deque<IZoneEvent*> _zoneEventListeners;

    /*
     * Index of the most recently activated zone.
     */
    int _currentZoneIndex = -1;

    /*
     * Used to ensure an imminent notification is only
     * generated once for a particular zone/threshold.
     *
     * We currently have one threshold. This can be expanded
     * when the second imminent notification is required.
     */
    int _imminentZoneIndex = -1;

    void UpdateZones(
        uint32_t gameTimeSeconds);

    void UpdatePickups(
        uint32_t gameTimeSeconds);

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

    ActiveGameObject* FindActiveObject(
        uint32_t objectId);

    const ActiveGameObject* FindActiveObject(
        uint32_t objectId) const;

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

    void RemoveActiveObject(
    uint32_t objectId);

    void Reset();        
};