#include "graphics/DisplayManager.h"
#include <memory>
#include "graphics/ZoomCommand.h"
#include "graphics/PlayerPositionCommand.h"
#include "graphics/TurnMapToHeadingCommand.h"
#include "graphics/DisplayBackgroundCommand.h"
#include "graphics/ConfigureMapCommand.h"
#include "graphics/AddWidgetCommand.h"
#include "widgets/MapWidget.h"
#include "widgets/ZoneWidget.h"
#include "widgets/SolidMapWidget.h"
#include "graphics/AddZoneCommand.h"
#include "graphics/RemoveWidgetCommand.h"
#include "graphics/SetWidgetColourCommand.h"
#include "graphics/SetZoneGeometryCommand.h"
#include "graphics/SetTextCommand.h"
#include "graphics/EnableWidgetCommand.h"
#include "graphics/SetTextureCommand.h"

DisplayManager::DisplayManager(
    TextureManager& textureManager)
    :
    _queueMutex(nullptr),
    _display(nullptr),
    _textureManager(textureManager),
    _heading(0.0f),
    _zoom(1.0f),
    _playerVisible(false),
    _renderEnabled(true)
{
}

DisplayManager::~DisplayManager()
{
    delete _display;

    if (_queueMutex != nullptr)
        vSemaphoreDelete(_queueMutex);
}

bool DisplayManager::Begin()
{
    _queueMutex =
        xSemaphoreCreateMutex();

    if (_queueMutex == nullptr)
    {
        Serial.println(
            "DisplayManager: failed to create queue mutex");

        return false;
    }

    _display =
        new ST7789VDisplay();

    if (_display == nullptr)
    {
        Serial.println(
            "DisplayManager: failed to create display");

        vSemaphoreDelete(_queueMutex);
        _queueMutex = nullptr;

        return false;
    }

    if (!_display->Init())
    {
        Serial.println(
            "DisplayManager: display init failed");

        delete _display;
        _display = nullptr;

        vSemaphoreDelete(_queueMutex);
        _queueMutex = nullptr;

        return false;
    }

    BaseType_t result =
        xTaskCreatePinnedToCore(
            DisplayTaskEntry,
            "DisplayTask",
            8192,
            this,
            1,
            nullptr,
            0);

    if (result != pdPASS)
    {
        Serial.println(
            "DisplayManager: failed to create display task");

        delete _display;
        _display = nullptr;

        vSemaphoreDelete(_queueMutex);
        _queueMutex = nullptr;

        return false;
    }

    return true;
}

bool DisplayManager::Send(
    std::unique_ptr<IDisplayCommand> command)
{
    if(command->GetType() == DisplayCommandType::RemoveWidget)
    {
        uint16_t id = static_cast<const RemoveWidgetCommand*>(command.get())->GetWidgetId();


        Serial.printf("Queing remove widget 0x%x\n", id);
    }
    if (!command ||
        _queueMutex == nullptr)
    {
        return false;
    }

    if (xSemaphoreTake(
            _queueMutex,
            portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    _commandQueue.push_back(
        std::move(command));

    xSemaphoreGive(
        _queueMutex);

    return true;
}

void DisplayManager::DisplayTaskEntry(
    void* parameter)
{
    DisplayManager* manager =
        static_cast<DisplayManager*>(parameter);

    Serial.println("Task started");

    manager->DisplayTask();

    /*
     * Should never return.
     */
    vTaskDelete(nullptr);
}

void DisplayManager::DisplayTask()
{
    for (;;)
    {
        /*
         * Take a snapshot of all commands waiting for
         * this display frame.
         *
         * The swap is protected by the mutex, but command
         * processing is deliberately performed outside it.
         */
        if (xSemaphoreTake(
                _queueMutex,
                portMAX_DELAY) == pdTRUE)
        {
            _processingQueue.swap(
                _commandQueue);

            xSemaphoreGive(
                _queueMutex);
        }

        /*
         * Process this frame's commands without holding
         * the queue mutex.
         */
        while (!_processingQueue.empty())
        {
            std::unique_ptr<IDisplayCommand> command =
                std::move(
                    _processingQueue.front());

            _processingQueue.pop_front();
            ProcessCommand(command.get());
    
        }

        uint32_t start = millis();

        if(_renderEnabled)
        {
            _display->Update(
                _playerPosition,
                _heading,
                _zoom,
                _playerVisible);
        }                

        uint32_t elapsed = millis() - start;

    if (elapsed > 500)
    {
        Serial.printf(
            "DisplayTask: Update took %lu ms\n",
            elapsed);
    }

        vTaskDelay(
            pdMS_TO_TICKS(10));
    }
}

void DisplayManager::ProcessCommand( IDisplayCommand* command)
{
   switch (command->GetType())
    {
        case DisplayCommandType::Zoom:
        {
            const ZoomCommand* zoomCommand =
                static_cast<const ZoomCommand*>(command);
               SetZoom(zoomCommand);

           
        }
        break;

        case DisplayCommandType::DisplayBackground:
        {
            const DisplayBackgroundCommand* backgroundCommand =
                static_cast<const DisplayBackgroundCommand*>(command);
                    
            DisplayBackground(backgroundCommand);
        }
        break;

        case DisplayCommandType::PlayerPosition:
        {
            const PlayerPositionCommand* positionCommand =
                static_cast<const PlayerPositionCommand*>(command);

            SetPlayerPosition(positionCommand);
         
        }
        break;

        case DisplayCommandType::TurnMapToHeading:
        {
            const TurnMapToHeadingCommand* headingCommand =
                static_cast<const TurnMapToHeadingCommand*>(command);

            TurnMapToHeading(headingCommand);
            
        }
        break;

        case DisplayCommandType::ConfigureMap:
        {
            const ConfigureMapCommand* configMapCommand =
                static_cast<const ConfigureMapCommand*>(command);

            ConfigureMap(configMapCommand);
            
        }
        break;

        case DisplayCommandType::AddWidget:
        {
            AddWidget(
                static_cast<AddWidgetCommand*>(command));

           
        }
        break;

        case DisplayCommandType::AddZone:
        {
            AddZone(
                static_cast<
                    const AddZoneCommand*>(
                        command));

          
        }
        break;

        case DisplayCommandType::RemoveWidget:
        {
            RemoveWidget(
                static_cast<
                    const RemoveWidgetCommand*>(
                        command));


        }
        break;

        case DisplayCommandType::SetWidgetColour:
        {
            SetWidgetColour(
                static_cast<
                    const SetWidgetColourCommand*>(
                        command));


        }
        break;

        case DisplayCommandType::SetZoneGeometry:
        {
            SetZoneGeometry(
                static_cast<
                    const SetZoneGeometryCommand*>(
                        command));


        }
        break;        

        case DisplayCommandType::SetTextCommand:
        {
            SetText(
                static_cast<
                    const SetTextCommand*>(
                        command));


        }
        break;   

        
        case DisplayCommandType::EnableWidgetCommand:
        {
            EnableWidget(
                static_cast<
                    const EnableWidgetCommand*>(
                        command));
        }
        break;   

        case DisplayCommandType::SetWidgetTexture:
        {
            SetWidgetTexture(
                static_cast<
                    const SetWidgetTextureCommand*>(
                        command));
        }
        break;

        default:
            Serial.printf("Unknown display command %d\n", command->GetType());
            break;
    }   
}


void DisplayManager::SetZoom(const ZoomCommand* command)
{
    _zoom = command->GetZoom();
}

void DisplayManager::SetPlayerPosition(const PlayerPositionCommand* command)
{
    _playerPosition = command->GetPosition();
    _playerVisible = true;
}

void DisplayManager::TurnMapToHeading(const TurnMapToHeadingCommand* command)
{
    
    _heading = command->GetHeading();   
}

void DisplayManager::DisplayBackground(
    const DisplayBackgroundCommand* command)
{
     Serial.printf("Setting background %s\n", command->GetFilename());
    if (!_display->SetBackground(command->GetFilename()))
    {
   
        Serial.printf(
            "DisplayManager: failed to set background %s\n",
            command->GetFilename());

        return;
    }

    /*
     * A new background starts with neutral navigation state.
     */
    _heading = 0.0f;
    _zoom = 1.0f;
    _playerVisible = false;

    Serial.printf(
        "DisplayManager: background %s\n",
        command->GetFilename());

}

void DisplayManager::ConfigureMap(const ConfigureMapCommand * command)
{
    _display->SetMapConfigData(command->GetMapConfigData());
}

void DisplayManager::AddWidget(AddWidgetCommand* command)
{
    if (command == nullptr)
        return;
        
    std::unique_ptr<Widget> widgetPtr= command->GetWidget();    
    if(widgetPtr != nullptr )
    {
        Serial.printf("DisplayManager::AddWidget %x\n", widgetPtr.get());
        _display->AddWidget(std::move(widgetPtr));
        return;
    }    
       
    switch (command->GetWidgetType())
    {
        case WidgetType::Pickup:
        {
            MapWidget* widget =
                new SolidMapWidget(
                    command->GetId(),
                    _textureManager,
                    command->GetPosition(),
                    command->GetTextureFilename().c_str(),  
                    command->GetPickupType());

            _display->AddWidget(std::unique_ptr<Widget>(widget));

            break;
        }
    }
}

void DisplayManager::AddZone(const AddZoneCommand* command)
{

    if (command == nullptr)
        return;

    ZoneWidget* widget =
        new ZoneWidget(
            command->GetId(),
            command->GetPosition(),
            command->GetRadius(),
            command->GetColour());

    _display->AddWidget(std::unique_ptr<Widget>(widget));
}

void DisplayManager::RemoveWidget(const RemoveWidgetCommand* command)
{
     if (command == nullptr)
        return;
    Serial.printf("Processing remove widget 0x%x\n", command->GetWidgetId());
    _display->RemovedWidget(command->GetWidgetId()); 
}

void DisplayManager::SetWidgetColour(const SetWidgetColourCommand* command)
{
    if (command == nullptr)
        return;

    _display->SetWidgetColour(command->GetWidgetId(), command->GetColour());        
}



void DisplayManager::SetZoneGeometry(const SetZoneGeometryCommand* command)
{
    if(command == nullptr)
        return;

    _display->SetZoneGeometry(command->GetId(), command->GetCoordinate() , command->GetRadius());
}

void DisplayManager::ConfigureNavigationMap(
    const char* mapFilename,
    double homeLatitude,
    double homeLongitude)
    {

        _display->ConfigureNavigationMap(mapFilename, homeLatitude, homeLongitude);
    }


void DisplayManager::SetText(const SetTextCommand* command)
{
    _display->SetText(command->GetWidgetId(), command->GetText());
}



void DisplayManager::EnableWidget(const EnableWidgetCommand* command)
{
    _display->EnableWidget(command->GetWidgetId(), command->Enabled());
}


void DisplayManager::SetWidgetTexture(
    const SetWidgetTextureCommand* command)
{
    Serial.println("DisplayManager::SetWidgetTexture");
    if (command == nullptr)
        return;
    Serial.printf("New filename %s\n",command->GetTextureFilename().c_str() );
    _display->SetWidgetTexture(
        command->GetWidgetId(),
        command->GetTextureFilename().c_str());
}