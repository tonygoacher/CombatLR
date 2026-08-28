#include "DisplayManager.h"
#include <memory>
#include "ZoomCommand.h"
#include "PlayerPositionCommand.h"
#include "TurnMapToHeadingCommand.h"
#include "DisplayBackgroundCommand.h"
#include "ConfigureMapCommand.h"

DisplayManager::DisplayManager(
    TextureManager& textureManager)
    :
    _queueMutex(nullptr),
    _display(nullptr),
    _textureManager(textureManager),
    _heading(0.0f),
    _zoom(1.0f),
    _playerVisible(false)
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

        _display->Update(
            _playerPosition,
            _heading,
            _zoom,
            _playerVisible);

        vTaskDelay(
            pdMS_TO_TICKS(10));
    }
}

void DisplayManager::ProcessCommand(const IDisplayCommand* command)
{
   switch (command->GetType())
    {
        case DisplayCommandType::Zoom:
        {
            const ZoomCommand* zoomCommand =
                static_cast<const ZoomCommand*>(command);
               SetZoom(zoomCommand);

            break;
        }

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
            break;
        }

        case DisplayCommandType::TurnMapToHeading:
        {
            const TurnMapToHeadingCommand* headingCommand =
                static_cast<const TurnMapToHeadingCommand*>(command);

            TurnMapToHeading(headingCommand);
            break;
        }

        case DisplayCommandType::ConfigureMap:
        {
            const ConfigureMapCommand* configMapCommand =
                static_cast<const ConfigureMapCommand*>(command);

            ConfigureMap(configMapCommand);
            break;
        }

        default:
            break;
    }   
}


void DisplayManager::SetZoom(const ZoomCommand* command)
{
    Serial.print("Zoom Command: "); Serial.println(command->GetZoom());
    _zoom = command->GetZoom();
}

void DisplayManager::SetPlayerPosition(const PlayerPositionCommand* command)
{
    _playerPosition = command->GetPosition();
    _playerVisible = true;
    Serial.printf("Player position %f %f\n", command->GetPosition().east, command->GetPosition().north);
}

void DisplayManager::TurnMapToHeading(const TurnMapToHeadingCommand* command)
{
    
    _heading = command->GetHeading();   
    Serial.printf("HEading %.3f\n", _heading);
}

void DisplayManager::DisplayBackground(
    const DisplayBackgroundCommand* command)
{
    const Texture* texture =
        _textureManager.GetTexture(
            command->GetFilename());

    if (texture == nullptr ||
        !texture->IsLoaded())
    {
        Serial.printf(
            "DisplayManager: failed to load background %s\n",
            command->GetFilename());

        return;
    }

    if (!_display->SetBackground(
            texture->GetData(),
            texture->GetWidth(),
            texture->GetHeight()))
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
        "DisplayManager: background %s %ux%u\n",
        command->GetFilename(),
        texture->GetWidth(),
        texture->GetHeight());
}

void DisplayManager::ConfigureMap(const ConfigureMapCommand * command)
{
    _display->SetMapConfigData(command->GetMapConfigData());
}

void DisplayManager::AddWidget(const AddWidgetCommand* command)
{
    
}

void DisplayManager::ConfigureNavigationMap(
    const char* mapFilename,
    double homeLatitude,
    double homeLongitude)
    {

        _display->ConfigureNavigationMap(mapFilename, homeLatitude, homeLongitude);
    }