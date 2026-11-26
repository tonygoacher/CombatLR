#include "statemachine/ProvisioningState.h"

#include "WiFi.h"
#include <esp_system.h>
#include "statemachine/SystemStates.h"
#include "statemachine/Services.h"
#include "SystemConfig.h"
#include "widgets/TextWidget.h"
#include "graphics/DisplayManager.h"
#include "graphics/DisplayBackgroundCommand.h"
#include "statemachine/Services.h"
#include "widgets/WiFiStatusWidget.h"
#include "widgets/UIWidgetIds.h"
#include "graphics/AddWidgetCommand.h"
#include "graphics/RemoveWidgetCommand.h"
#include "graphics/SetTextureCommand.h"
#include "graphics/SetTextCommand.h"
#include "widgets/TextureWidget.h"
#include "TeamIDs.h"
#include "widgets/TextWidget.h"


ProvisioningState::ProvisioningState(CombatLRServices& services)
    : AwaitingStartState(services),
      _webServer(nullptr),
      _webServerStarted(false),
      _saveRequested(false),
      _apIP(192, 168, 4, 1),
      _apPassword()
{
}

void ProvisioningState::Entry()
{
    Serial.println();
    Serial.println("=================================");
    Serial.println("CombatLR network provisioning mode");
    Serial.println("=================================");

    WidgetPosition pos(320-32,0);

    TextureWidget* statusWidget = new TextureWidget(UIWidgetIds::WiFiStatus,
         _services.displayManager.GetTextureManager(),pos, "/provision.bmp");

    _services.displayManager.Send(std::make_unique<AddWidgetCommand>(statusWidget));   

    WidgetPosition textPos={TextX,TextY};

    TextWidget*  textWidget  = new TextWidget(UIWidgetIds::IPAdrString,"xx.xx.xx.xx", textPos,TextFont,MC_DATUM, TFT_WHITE, TFT_BLACK, TextZoom);
    
    _services.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(
                    textWidget)));

    WidgetPosition keyTextPos={TextX,TextY+44};
    TextWidget* keyWidget = new TextWidget(
        UIWidgetIds::SSIDString,"",keyTextPos,TextFont,MC_DATUM,
        TFT_WHITE,TFT_BLACK,TextZoom);
    _services.displayManager.Send(
        std::unique_ptr<IDisplayCommand>(new AddWidgetCommand(keyWidget)));

    _saveRequested = false;
    _webServerStarted = false;

    StartAccessPoint();
    StartWebServer();
}

void ProvisioningState::Exit()
{
    StopWebServer();

    Serial.println("Stopping provisioning AP");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    RemoveWidgetCommand* removeIcon = new RemoveWidgetCommand(UIWidgetIds::WiFiStatus);

    _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(removeIcon));    
    removeIcon = new RemoveWidgetCommand(UIWidgetIds::IPAdrString);
    _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(removeIcon));

    removeIcon = new RemoveWidgetCommand(UIWidgetIds::SSIDString);
    _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(removeIcon));
}

uint16_t ProvisioningState::Run()
{
    if (_webServerStarted && _webServer != nullptr)
        _webServer->handleClient();

    if (_saveRequested)
        return SystemStates::GPSLock;

    return SystemStates::Provisioning;
}

void ProvisioningState::StartAccessPoint()
{
    WiFi.mode(WIFI_AP);

    const uint32_t chipId = static_cast<uint32_t>(ESP.getEfuseMac());
    const uint32_t setupKey = esp_random();
    char suffix[5];
    char password[9];
    snprintf(suffix, sizeof(suffix), "%04X", static_cast<unsigned>(chipId & 0xFFFF));
    snprintf(password, sizeof(password), "%08X", static_cast<unsigned>(setupKey));
    _apPassword = password;

    String apName = String("CombatLR-") + suffix;

    if (!WiFi.softAPConfig(_apIP, _apIP, IPAddress(255, 255, 255, 0)))
        Serial.println("Provisioning: failed to configure AP IP");

    if (!WiFi.softAP(apName.c_str(), _apPassword.c_str()))
    {
        Serial.println("Provisioning: failed to start AP");
        return;
    }

    Serial.print("Provisioning AP: ");
    Serial.println(apName);
    Serial.print("Open: http://");
    Serial.println(WiFi.softAPIP());

    _services.displayManager.Send(
        std::make_unique<SetTextCommand>(UIWidgetIds::IPAdrString, WiFi.softAPIP().toString().c_str()));

    String keyText = String("Setup key: ") + _apPassword;
    _services.displayManager.Send(
        std::make_unique<SetTextCommand>(UIWidgetIds::SSIDString, keyText.c_str()));
}

void ProvisioningState::StartWebServer()
{
    if (_webServer != nullptr)
        return;

    _webServer = new WebServer(80);

    _webServer->on("/", HTTP_GET, [this]()
    {
        HandleRoot();
    });

    _webServer->on("/save", HTTP_POST, [this]()
    {
        HandleSave();
    });

    _webServer->onNotFound([this]()
    {
        HandleNotFound();
    });

    _webServer->begin();
    _webServerStarted = true;

    Serial.println("Provisioning web server started");
}

void ProvisioningState::StopWebServer()
{
    if (_webServer != nullptr)
    {
        _webServer->stop();
        delete _webServer;
        _webServer = nullptr;
    }

    _webServerStarted = false;
}

void ProvisioningState::HandleRoot()
{
    if (_webServer == nullptr)
        return;

    _webServer->send(200, "text/html; charset=utf-8", BuildPage());
}

void ProvisioningState::HandleSave()
{
    if (_webServer == nullptr)
        return;

    String ssids[8];
    String passwords[8];
    uint8_t count = 0;

    for (uint8_t i = 0; i < 8; ++i)
    {
        String ssidName = "ssid";
        ssidName += i;

        String passName = "pass";
        passName += i;

        String ssid = GetArg(ssidName.c_str());
        String password = GetArg(passName.c_str());

        ssid.trim();

        if (ssid.length() == 0)
            continue;

        // Blank password retains the existing password for the same SSID.
        if (password.length() == 0 &&
            i < _services.systemConfig.GetWifiCount() &&
            ssid == _services.systemConfig.GetWifiSSID(i))
        {
            password = _services.systemConfig.GetWifiPass(i);
        }

        if (password.length() == 0)
        {
            _webServer->send(400, "text/html; charset=utf-8",
                "<html><body><h2>Error</h2><p>Enter a password for each new Wi-Fi network.</p>"
                "<p><a href='/'>Back</a></p></body></html>");
            return;
        }

        ssids[count] = ssid;
        passwords[count] = password;
        ++count;
    }

    if (count == 0)
    {
        _webServer->send(400, "text/html; charset=utf-8",
            "<html><body><h2>Error</h2><p>At least one Wi-Fi network is required.</p>"
            "<p><a href='/'>Back</a></p></body></html>");
        return;
    }

    String ftpUser = GetArg("ftpUser");
    String ftpPass = GetArg("ftpPass");
    if (ftpPass.length() == 0)
        ftpPass = _services.systemConfig.GetFtpPass();

    if (ftpUser.length() == 0 || ftpPass.length() == 0)
    {
        _webServer->send(400, "text/html; charset=utf-8",
            "<html><body><h2>Error</h2><p>FTP username and password are required.</p>"
            "<p><a href='/'>Back</a></p></body></html>");
        return;
    }

    if (!_services.systemConfig.SetProvisioningConfig(
            ssids, passwords, count, ftpUser, ftpPass))
    {
        _webServer->send(500, "text/html; charset=utf-8",
            "<html><body><h2>Save failed</h2>"
            "<p>Could not save credentials to NVS.</p>"
            "<p><a href='/'>Back</a></p></body></html>");
        return;
    }

    Serial.println("Provisioning: configuration saved");

    _webServer->send(200, "text/html; charset=utf-8",
        "<html><head><meta http-equiv='refresh' content='3;url=/'>"
        "<style>body{font-family:Arial;text-align:center;margin-top:80px}</style>"
        "</head><body><h2>Configuration saved</h2>"
        "<p>The ESP32 will return to normal startup.</p></body></html>");

    _saveRequested = true;
}

void ProvisioningState::HandleNotFound()
{
    if (_webServer == nullptr)
        return;

    _webServer->send(404, "text/plain", "Not found");
}

String ProvisioningState::GetArg(const char* name)
{
    if (_webServer == nullptr || !_webServer->hasArg(name))
        return String();

    return _webServer->arg(name);
}

String ProvisioningState::HtmlEscape(const String& value)
{
    String result;
    result.reserve(value.length() + 16);

    for (size_t i = 0; i < value.length(); ++i)
    {
        switch (value[i])
        {
            case '&': result += "&amp;";  break;
            case '<': result += "&lt;";   break;
            case '>': result += "&gt;";   break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += value[i]; break;
        }
    }

    return result;
}

String ProvisioningState::BuildPage()
{
    String page;
    page.reserve(9000);

    page += F(R"HTML(<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CombatLR Network Setup</title>
<style>
body{font-family:Arial,sans-serif;background:#202020;color:#eee;margin:0;padding:20px}
.container{max-width:700px;margin:auto}
h1{font-size:26px}
h2{font-size:20px;margin-top:28px}
.card{background:#303030;border-radius:8px;padding:16px;margin-bottom:16px}
label{display:block;margin-top:10px;margin-bottom:4px}
input{box-sizing:border-box;width:100%;padding:10px;border:1px solid #666;border-radius:4px;background:#181818;color:#fff;font-size:16px}
.network{border-top:1px solid #555;padding-top:10px;margin-top:14px}
button{width:100%;padding:13px;margin-top:22px;border:0;border-radius:5px;font-size:17px;font-weight:bold}
.save{background:#ddd;color:#111}
.small{color:#aaa;font-size:14px}
</style>
</head>
<body>
<div class="container">
<h1>CombatLR Network Setup</h1>
<div class="card">
<p class="small">Configure Wi-Fi networks and FTP credentials. Secrets are stored separately in device NVS.</p>
<form method="POST" action="/save">
<h2>Wi-Fi Networks</h2>
)HTML");

    const uint8_t count = _services.systemConfig.GetWifiCount();

    for (uint8_t i = 0; i < 8; ++i)
    {
        const bool existing = i < count;
        const String ssid = existing ? _services.systemConfig.GetWifiSSID(i) : String();
        page += "<div class='network'><b>Network ";
        page += String(i + 1);
        page += "</b>";

        page += "<label>SSID</label><input name='ssid";
        page += String(i);
        page += "' value='";
        page += HtmlEscape(ssid);
        page += "' maxlength='63'>";

        page += "<label>Password</label><input type='password' name='pass";
        page += String(i);
        page += "' value='";
        // Never send stored passwords back to the browser.
        page += "' placeholder='Leave blank to keep existing password' maxlength='63'>";
        page += "</div>";
    }

    page += F(R"HTML(
<h2>FTP</h2>
<label>Username</label>
<input name="ftpUser" value=")HTML");
    page += HtmlEscape(_services.systemConfig.GetFtpUser());
    page += F(R"HTML(" maxlength="32">
<label>Password</label>
<input type="password" name="ftpPass" value=")HTML");
    page += F(R"HTML(" placeholder="Leave blank to keep existing password" maxlength="64">
<button class="save" type="submit">Save Configuration</button>
</form>
</div>
</div>
</body>
</html>
)HTML");

    return page;
}
