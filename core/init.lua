-- made by @sleppy.lol._05074 (kittyy) and core modules execution by @quangaledingle (Flying_DUM12345)
-- special thanks to @byteblue for fixing loadstring and good httpget/httppost funcs (loadstring function will be released in next update)

repeat task.wait(.01) until game:IsLoaded()

local http_service = game:GetService("HttpService")
local http_rbx = game:GetService("HttpRbxApiService")
local core_gui = game:GetService("CoreGui")
local starter_gui = game:GetService("StarterGui")
local user_input = game:GetService("UserInputService")
local sound_service = game:GetService("SoundService")

local client_pid = "%client_pid%"
local client_port = "%client_port%"
local client_version = "%client_version%"
local client_executorname = "%client_name%"

if core_gui:FindFirstChild(client_pid) then return end

local container = Instance.new("Folder", core_gui)
container.Name = client_pid

local server_url = "http://127.0.0.1:" .. client_port

-- server sender
local function sendData(path, body)
    local done, status, response = false, nil, nil
    http_service:RequestInternal({
        Url = server_url .. path,
        Method = "POST",
        Body = body or "",
        Headers = { ["Content-Type"] = "text/plain" }
    }):Start(function(success, resp)
        done = true
        if success then
            status = resp.StatusCode
            response = resp.Body
        end
    end)
    local t0 = tick()
    while not done and tick() - t0 < 5 do task.wait() end
    return status, response
end

local function sendJsonData(path, body)
    local done, status, respBody = false, nil, nil
    http_service:RequestInternal({
        Url = server_url .. path,
        Method = "POST",
        Body = body or "",
        Headers = { ["Content-Type"] = "application/json" }
    }):Start(function(success, response)
        done = true
        if success then
            status = response.StatusCode
            respBody = response.Body
        end
    end)
    local t0 = tick()
    while not done and tick() - t0 < 30 do task.wait() end
    return status, respBody
end

local function fs_request(endpoint, body)
    local done, respBody = false, nil
    http_service:RequestInternal({
        Url = server_url .. "/fs/" .. endpoint,
        Method = "POST",
        Body = body or "",
        Headers = { ["Content-Type"] = "text/plain" }
    }):Start(function(success, response)
        done = true
        if success then respBody = response.Body end
    end)
    local t0 = tick()
    while not done and tick() - t0 < 5 do task.wait() end
    return respBody or ""
end

local blocked_domains = {
    ["accountinformation.roblox.com"] = true, ["accountsettings.roblox.com"] = true,
    ["twostepverification.roblox.com"] = true, ["trades.roblox.com"] = true,
    ["billing.roblox.com"] = true, ["economy.roblox.com"] = true, ["auth.roblox.com"] = true,
    ["accountinformation.roproxy.com"] = true, ["accountsettings.roproxy.com"] = true,
    ["twostepverification.roproxy.com"] = true, ["trades.roproxy.com"] = true,
    ["billing.roproxy.com"] = true, ["economy.roproxy.com"] = true, ["auth.roproxy.com"] = true,
    ["api.ipify.org"] = true, ["api4.ipify.org"] = true, ["api6.ipify.org"] = true, ["api64.ipify.org"] = true,
    ["api.ipapi.is"] = true, ["api.ipapi.co"] = true, ["api.ipapi.com"] = true,
    ["api.ipinfo.io"] = true, ["api.ipgeolocation.io"] = true, ["api.freegeoip.app"] = true,
    ["api.myip.com"] = true, ["api.ip.sb"] = true, ["api.ipwhois.io"] = true,
    ["api.ipdata.co"] = true, ["api.ipstack.com"] = true, ["api.db-ip.com"] = true,
    ["api.seeip.org"] = true, ["api.ipregistry.co"] = true, ["api.abstractapi.com"] = true,
    ["api.ipbase.com"] = true, ["api.iplocation.net"] = true, ["api.ip.nf"] = true,
    ["api.bigdatacloud.net"] = true, ["api.techniknews.net"] = true, ["api.country.is"] = true,
    ["api.2ip.me"] = true, ["api.reallyfreegeoip.org"] = true,
}

local function is_url_allowed(url)
    if type(url) ~= "string" then 
        error("Attempt to request a blocked URL 'invalid type'", 0) 
    end
    local lower = string.lower(url)
	if string.sub(lower, 1, 7) == "file://" then error("Attempt to request a blocked URL 'file://'", 0) end
	local host = string.match(lower, "^%a+://([^/]+)") or ""
	if host == "" then return end 
	host = string.gsub(host, ":%d+$", "")
	if blocked_domains[host] then error("Attempt to request a blocked Roblox domain", 0) end
end
-- end

-- core modules searcher (thanks to Flying_DUM12345 for realisation)
local function findModule(root)
    for _, child in ipairs(root:GetDescendants()) do
        if child:IsA("ModuleScript") then
            return child
        end
    end
    return nil
end

local function copyModule(name, parent)
    local packages = game:GetService("CorePackages"):FindFirstChild("Packages")
    local module = findModule(packages)
    if not module then warn("module script not found in packages") end
    local inst = module:Clone()
    inst.Name = name
    inst.Parent = parent or container
    return inst
end
-- end

local kitty_env = getfenv(0) -- main environment table

-- httpget/httppost/getobjects thanks to @byteblue for simple httpget/httppost functions
local function httpGet(self, url)
    is_url_allowed(url)
    local actual_url = type(self) == "string" and self or url
    if not actual_url or type(actual_url) ~= "string" or actual_url == "" then return "" end
    
    local ok, res = pcall(function()
        return http_service:RequestAsync({ Url = actual_url, Method = "GET" })
    end)
    if ok and res.Success then
        return res.Body
    end
    
    local done, result = false, nil
    http_service:RequestInternal({ Url = actual_url, Method = "GET" }):Start(function(success, response)
        done = true
        if success and response.StatusCode == 200 then result = response.Body end
    end)
    local t0 = tick()
    while not done and tick() - t0 < 10 do task.wait() end
    return result or ""
end

local function httpPost(url, body, contentType)
    is_url_allowed(url)
    local actual_url = type(self) == "string" and self or url
    if not actual_url or type(actual_url) ~= "string" or actual_url == "" then return "" end
    
    local ok, res = pcall(function()
        return http_service:RequestAsync({ Url = actual_url, Method = "POST", Body = body or "", Headers = { ["Content-Type"] = contentType or "application/json" } })
    end)
    if ok and res.Success then
        return res.Body
    end
    
    local done, result = false, nil
    http_service:RequestInternal({ Url = actual_url, Method = "POST", Body = body or "", Headers = { ["Content-Type"] = contentType or "application/json" } }):Start(function(success, response)
        done = true
        if success and response.StatusCode == 200 then result = response.Body end
    end)
    local t0 = tick()
    while not done and tick() - t0 < 10 do task.wait() end
    return result or ""
end

local function getObjects(asset)
	return {game:GetService("InsertService"):LoadLocalAsset(asset)}
end
-- end

-- game proxier
local _game = game -- save og game instance

local proxiedServices = {
	Stats = {{
		"GetMemoryUsageMbForTag", "GetTotalMemoryUsageMb",
		"GetPaginatedMemoryByTexture", "GetMemoryUsageMbForTextureTag",
		"GetTextureMemoryUsageMb", "GetPhysicsStats", "GetNetworkStats",
		"GetStatsItem", "GetTaggedMemoryUsageMb",
	}, _game:GetService("Stats")},

	BrowserService = {{
		"EmitHybridEvent", "OpenWeChatAuthWindow", "ExecuteJavaScript",
		"OpenBrowserWindow", "OpenNativeOverlay", "ReturnToJavaScript",
		"CopyAuthCookieFromBrowserToEngine", "SendCommand", "OpenUrl", "DetectUrl"
	}, _game:GetService("BrowserService")},

	ScriptContext = {{
		"AddCoreScriptLocal", "SaveScriptProfilingData"
	}, _game:GetService("ScriptContext")},

	ScriptProfilerService = {{
		"SaveScriptProfilingData"
	}, _game:GetService("ScriptProfilerService")},

	MicroProfilerService = {{}, _game:GetService("MicroProfilerService")},

	TestService = {{
		"Run", "RunAsync", "Require"
	}, _game:GetService("TestService")},

	AccountService = {{
		"GetCredentialsHeaders", "GetDeviceAccessToken",
		"GetDeviceIntegrityToken", "GetDeviceIntegrityTokenYield"
	}, _game:GetService("AccountService")},

	MessageBusService = {{
		"Call", "GetLast", "GetMessageId", "GetProtocolMethodRequestMessageId",
		"GetProtocolMethodResponseMessageId", "MakeRequest", "Publish",
		"PublishProtocolMethodRequest", "PublishProtocolMethodResponse",
		"Subscribe", "SubscribeToProtocolMethodRequest", "SubscribeToProtocolMethodResponse"
	}, _game:GetService("MessageBusService")},

	OpenCloudService = {{
		"HttpRequestAsync"
	}, _game:GetService("OpenCloudService")},

	HttpRbxApiService = {{
		"PostAsync", "PostAsyncFullUrl", "GetAsyncFullUrl",
		"GetAsync", "RequestAsync", "RequestLimitedAsync"
	}, _game:GetService("HttpRbxApiService")},

	AvatarEditorService = {{
		"NoPromptSetFavorite", "NoPromptUpdateOutfit", "PerformCreateOutfitWithDescription",
		"PerformDeleteOutfit", "PerformRenameOutfit", "PerformSaveAvatarWithDescription",
		"PerformSetFavorite", "PerformUpdateOutfit", "PromptAllowInventoryReadAccess",
		"PromptCreateOutfit", "PromptDeleteOutfit", "PromptRenameOutfit",
		"PromptSaveAvatar", "PromptSetFavorite", "PromptUpdateOutfit",
		"SetAllowInventoryReadAccess", "SignalCreateOutfitFailed",
		"SignalCreateOutfitPermissionDenied", "SignalDeleteOutfitFailed",
		"SignalDeleteOutfitPermissionDenied", "SignalRenameOutfitFailed",
		"SignalRenameOutfitPermissionDenied", "SignalSaveAvatarPermissionDenied",
		"SignalSetFavoriteFailed", "SignalSetFavoritePermissionDenied",
		"SignalUpdateOutfitFailed", "SignalUpdateOutfitPermissionDenied",
		"NoPromptSaveAvatarThumbnailCustomization", "NoPromptSaveAvatar",
		"NoPromptRenameOutfit", "NoPromptDeleteOutfit", "NoPromptCreateOutfit"
	}, _game:GetService("AvatarEditorService")},

	CaptureService = {{
		"DeleteCapture", "DeleteCapturesAsync", "GetCaptureFilePathAsync", "CreatePostAsync",
		"SaveCaptureToExternalStorage", "SaveCapturesToExternalStorageAsync",
		"GetCaptureSizeAsync", "GetCaptureStorageSizeAsync", "GetCaptureUploadDataAsync",
		"PromptSaveCapturesToGallery", "PromptShareCapture", "CaptureScreenshot",
		"RetrieveCaptures", "SaveScreenshotCapture"
	}, _game:GetService("CaptureService")},

	LinkingService = {{
		"DetectUrl", "GetAndClearLastPendingUrl", "GetLastLuaUrl",
		"IsUrlRegistered", "OpenUrl", "RegisterLuaUrl",
		"StartLuaUrlDelivery", "StopLuaUrlDelivery",
		"SupportsSwitchToSettingsApp", "SwitchToSettingsApp"
	}, _game:GetService("LinkingService")},

	HttpService = {{
		"RequestInternal", "GetAsync", "RequestAsync", "PostAsync", "SetHttpEnabled"
	}, _game:GetService("HttpService")},

	MarketplaceService = {{
		"PerformCancelSubscription", "PerformPurchaseV2", "PrepareCollectiblesPurchase",
		"PromptCancelSubscription", "ReportAssetSale", "GetUserSubscriptionDetailsInternalAsync",
		"PerformPurchase", "PromptBundlePurchase", "PromptGamePassPurchase",
		"PromptProductPurchase", "PromptPurchase", "PromptRobloxPurchase",
		"PromptThirdPartyPurchase", "GetRobuxBalance", "PromptBulkPurchase",
		"PerformBulkPurchase", "PerformSubscriptionPurchase", "PerformSubscriptionPurchaseV2",
		"PromptCollectiblesPurchase", "PromptNativePurchase", "PromptNativePurchaseWithLocalPlayer",
		"PromptPremiumPurchase", "PromptSubscriptionPurchase",
		"GetUserSubscriptionPaymentHistoryAsync", "GetUserSubscriptionStatusAsync"
	}, _game:GetService("MarketplaceService")},

	GuiService = {{
		"OpenBrowserWindow", "OpenNativeOverlay", "BroadcastNotification", "SetPurchasePromptIsShown"
	}, _game:GetService("GuiService")},

	EventIngestService = {{
		"SendEventDeferred", "SetRBXEvent", "SetRBXEventStream", "SendEventImmediately"
	}, _game:GetService("EventIngestService")},

	CoreScriptSyncService = {{
		"GetScriptFilePath"
	}, _game:GetService("CoreScriptSyncService")},

	AssetService = {{
		"RegisterUGCValidationFunction"
	}, _game:GetService("AssetService")},

	ContentProvider = {{
		"SetBaseUrl"
	}, _game:GetService("ContentProvider")},

	AppStorageService = {{
		"Flush", "GetItem", "SetItem"
	}, _game:GetService("AppStorageService")},

	IXPService = {{
		"GetBrowserTrackerLayerVariables", "GetRegisteredUserLayersToStatus",
		"GetUserLayerVariables", "GetUserStatusForLayer", "InitializeUserLayers",
		"LogBrowserTrackerLayerExposure", "LogUserLayerExposure", "RegisterUserLayers"
	}, _game:GetService("IXPService")},

	SessionService = {{
		"AcquireContextFocus", "GenerateSessionInfoString", "GetCreatedTimestampUtcMs",
		"GetMetadata", "GetRootSID", "GetSessionTag", "IsContextFocused",
		"ReleaseContextFocus", "RemoveMetadata", "RemoveSession",
		"RemoveSessionsWithMetadataKey", "ReplaceSession", "SessionExists",
		"SetMetadata", "SetSession", "GetSessionID"
	}, _game:GetService("SessionService")},

	ContextActionService = {{
		"CallFunction", "BindCoreActivate"
	}, _game:GetService("ContextActionService")},

	CommerceService = {{
		"PromptCommerceProductPurchase", "PromptRealWorldCommerceBrowser",
		"UserEligibleForRealWorldCommerceAsync"
	}, _game:GetService("CommerceService")},

	Players = {{
		"ReportAbuse", "ReportAbuseV3", "ReportChatAbuse"
	}, _game:GetService("Players")},

	PlatformCloudStorageService = {{
		"GetUserDataAsync", "SetUserDataAsync"
	}, _game:GetService("PlatformCloudStorageService")},

	CoreGui = {{
		"TakeScreenshot", "ToggleRecording"
	}, _game:GetService("CoreGui")},

	RbxAnalyticsService = {{
		"GetSessionId", "ReleaseRBXEventStream", "SendEventDeferred",
		"SendEventImmediately", "SetRBXEvent", "SetRBXEventStream",
		"TrackEvent", "TrackEventWithArgs"
	}, _game:GetService("RbxAnalyticsService")},

	FileManagerService = {{
		"ListFilesInFolderAsync", "OpenFileInWebBrowser", "OpenFolder", "RevealFileInFolder"
	}, _game:GetService("FileManagerService")},

	WebSocketService = {{
		"CreateClient"
	}, _game:GetService("WebSocketService")},
	
	SafetyService = {{
		"TakeScreenshot"
	}, _game:GetService("SafetyService")},

	AnimationFromVideoCreatorService = {{
		"CreateJob", "DownloadJobResult", "FullProcess"
	}, _game:GetService("AnimationFromVideoCreatorService")},

	AppUpdateService = {{
		"PerformManagedUpdate"
	}, _game:GetService("AppUpdateService")},

	MessagingService = {{
		"PublishAsync", "SubscribeAsync"
	}, _game:GetService("MessagingService")},

	OmniRecommendationsService = {{
		"ClearSessionId", "MakeRequest"
	}, _game:GetService("OmniRecommendationsService")},
}

local function find(t, x)
	x = string.gsub(tostring(x), '\0', '')
	for i, v in ipairs(t) do
		if v:lower() == x:lower() then
			return true
		end
	end
	return false
end

local function blockServiceFuncs(serviceTable)
	serviceTable.proxy = newproxy(true)
	local proxyMt = getmetatable(serviceTable.proxy)

	proxyMt.__index = function(self, index)
		index = string.gsub(tostring(index), '\0', '')
		
		if find(serviceTable[1], index) then
			return function(self, ...)
				error("Attempt to call a blocked function: " .. index, 2)
			end
		end

		if index == "Parent" then
			return kitty_env.game
		end

		if type(serviceTable[2][index]) == "function" then
			return function(self, ...)
				return serviceTable[2][index](serviceTable[2], ...)
			end
		else
			return serviceTable[2][index]
		end
	end

	proxyMt.__newindex = function(self, index, value)
		serviceTable[2][index] = value
	end

	proxyMt.__tostring = function(self)
		return serviceTable[2].Name
	end

	proxyMt.__metatable = getmetatable(serviceTable[2])
end

kitty_env.game = newproxy(true)
local gProxy = getmetatable(kitty_env.game)

gProxy.__index = function(self, index)
	index = string.gsub(tostring(index), '\0', '')

	if index == "HttpGet" or index == "HttpGetAsync" then
		return httpGet
	elseif index == "HttpPost" or index == "HttpPostAsync" then
		return httpPost
	elseif index == "GetObjects" then
		return function(self, ...) return getObjects(...) end
	end

	if proxiedServices[index] then
		return proxiedServices[index].proxy
	end

	if type(_game[index]) == "function" then
		return function(self, ...)
			if index == "GetService" or index == "FindService" then
				local args = {...}
				local service_name = string.gsub(tostring(args[1]), '\0', '')
				
				if proxiedServices[service_name] then
					return proxiedServices[service_name].proxy
				end
			end
			
			if find({
				"Load",
				"OpenScreenshotsFolder",
				"OpenVideosFolder"
			}, index) then
				error("Attempt to call a blocked function: " .. tostring(index), 2)
			end
			
			return _game[index](_game, ...)
		end
	else
		if proxiedServices[index] then
			return proxiedServices[index].proxy
		end
		return _game[index]
	end
end

gProxy.__newindex = function(self, index, value)
	_game[index] = value
end

gProxy.__tostring = function(self)
	return _game.Name
end

gProxy.__metatable = getmetatable(_game)

kitty_env.Game = kitty_env.game

for i, serviceTable in pairs(proxiedServices) do
	blockServiceFuncs(serviceTable)
end
-- end

-- unc functions
kitty_env.getgenv = function()
    return kitty_env
end

local renv_table = {
	game = kitty_env.game, -- proxied game instance
	workspace = kitty_env.workspace,
	script = kitty_env.script,
	Instance = kitty_env.Instance,
	Enum = kitty_env.Enum,
	task = kitty_env.task,
	coroutine = kitty_env.coroutine,
	print = kitty_env.print,
	warn = kitty_env.warn,
	error = kitty_env.error,
	assert = kitty_env.assert,
	pcall = kitty_env.pcall,
	xpcall = kitty_env.xpcall,
	select = kitty_env.select,
	tonumber = kitty_env.tonumber,
	tostring = kitty_env.tostring,
	type = kitty_env.type,
	typeof = kitty_env.typeof,
	tick = kitty_env.tick,
	wait = kitty_env.wait,
	spawn = kitty_env.spawn,
	delay = kitty_env.delay,
	require = kitty_env.require,
	math = kitty_env.math,
	string = kitty_env.string,
	table = kitty_env.table,
	os = kitty_env.os,
	bit32 = kitty_env.bit32,
	buffer = kitty_env.buffer,
	utf8 = kitty_env.utf8,
	_G = {},
}

kitty_env.getrenv = function()
	return renv_table
end

kitty_env.getreg = function()
	return {[1] = coroutine.running(),_G = _G}
end
kitty_env.getregistry = kitty_env.getreg

local hidden_ui = nil
kitty_env.gethui = function()
	if hidden_ui and hidden_ui.Parent then return hidden_ui end
	local ok, gui = pcall(function()
		local screen_gui = Instance.new('ScreenGui', core_gui)
		screen_gui.Name = "RobloxUI"
		screen_gui.ResetOnSpawn = false
		screen_gui.DisplayOrder = 999999
		screen_gui.IgnoreGuiInset = true
		return screen_gui
	end)
	if ok then hidden_ui = gui end
	return hidden_ui
end
kitty_env.get_hidden_gui = kitty_env.gethui

-- executor identifier
kitty_env.identifyexecutor = function()
    return client_executorname,client_version
end

kitty_env.getexecutorname = function()
	local name, _ = kitty_env.identifyexecutor()
    return name
end

kitty_env.getexecutorversion = function()
	local _, version = kitty_env.identifyexecutor()
    return version
end

kitty_env.gethwid = function()
    local status, response = sendData("/gethwid?p="..client_pid,"")
    if status ~= 200 and typeof(response) ~= "string" then
        return ""
    end
    return response
end

kitty_env.setfpscap = function(fps)
    assert(typeof(fps) == "number", "invalid argument #1 to 'setfpscap' (number expected, got " .. type(fps) .. ')', 2)
	local status, response = sendData("/setfpscap?p=" .. client_pid .. "&n="..tostring(math.floor(fps + 0.5)),"")
	if status ~= 200 and response ~= 'ok' then
		return false
	end
    return true
end

kitty_env.getfpscap = function()
	local status, response = sendData("/getfpscap?p=" .. client_pid,"")
	local cap = tonumber(response)
	if status ~= 200 and not cap then
		return nil
	end
	return cap
end

kitty_env.setclipboard = function(text)
    assert(typeof(text) == "string", "invalid argument #1 to 'setclipboard' (string expected, got " .. type(text) .. ')', 2)
	local status, response = sendData("/setclipboard?p=" .. client_pid .. "&t="..text,"")
	if status ~= 200 and response ~= 'ok' then
		return false
	end
    return true
end
kitty_env.setrbxclipboard = kitty_env.setclipboard
kitty_env.toclipboard = kitty_env.setclipboard

kitty_env.getclipboard = function()
	local status, response = sendData("/getclipboard?p=" .. client_pid,"")
    if status ~= 200 and response == '' then
        return ""
    end
    return response
end
kitty_env.getrbxclipboard = kitty_env.getclipboard
kitty_env.fromclipboard = kitty_env.getclipboard

local rbx_active = false

user_input.WindowFocused:Connect(function()
    rbx_active = true
end)

user_input.WindowFocusReleased:Connect(function()
    rbx_active = false
end)

kitty_env.isrbxactive = function()
    return rbx_active
end
kitty_env.isgameactive = kitty_env.isrbxactive
kitty_env.iswindowactive = kitty_env.isrbxactive
-- end

-- crypt

-- base64 encoder/decoder
local b64 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'

function base64encode(data)
    data = tostring(data or '')
    local out = {}
    for i = 1, #data, 3 do
        local a, b, c = data:byte(i, i + 2)
        local n = bit32.bor(
            bit32.lshift(a, 16),
            bit32.lshift(b or 0, 8),
            c or 0
        )
        out[#out + 1] = b64:sub(bit32.band(bit32.rshift(n, 18), 63) + 1, bit32.band(bit32.rshift(n, 18), 63) + 1)
                      .. b64:sub(bit32.band(bit32.rshift(n, 12), 63) + 1, bit32.band(bit32.rshift(n, 12), 63) + 1)
        out[#out + 1] = b and b64:sub(bit32.band(bit32.rshift(n, 6), 63) + 1, bit32.band(bit32.rshift(n, 6), 63) + 1) or '='
        out[#out + 1] = c and b64:sub(bit32.band(n, 63) + 1, bit32.band(n, 63) + 1) or '='
    end
    return table.concat(out)
end

function base64decode(data)
    data = tostring(data or ''):gsub('[^%w+/=]', '')
    local out = {}
    for i = 1, #data, 4 do
        local a, b, c, d = data:byte(i, i + 3)
        local n = bit32.bor(
            bit32.lshift((b64:find(string.char(a), 1, true) - 1), 18),
            bit32.lshift(((b and b64:find(string.char(b), 1, true) or 1) - 1), 12),
            bit32.lshift(((c and c ~= 61 and b64:find(string.char(c), 1, true) or 1) - 1), 6),
            ((d and d ~= 61 and b64:find(string.char(d), 1, true) or 1) - 1)
        )
        out[#out + 1] = string.char(bit32.band(bit32.rshift(n, 16), 255))
        if c and c ~= 61 then out[#out + 1] = string.char(bit32.band(bit32.rshift(n, 8), 255)) end
        if d and d ~= 61 then out[#out + 1] = string.char(bit32.band(n, 255)) end
    end
    return table.concat(out)
end

kitty_env.crypt = {
    base64encode = base64encode,
    base64decode = base64decode,
    base64_encode = base64encode,
    base64_decode = base64decode,
    base64 = { encode = base64encode, decode = base64decode },
}

kitty_env.base64 = { encode = base64encode, decode = base64decode }
base64_encode = base64encode
base64_decode = base64decode

local charset = {}
for a = 33, 57 do charset[#charset + 1] = string.char(a) end
for a = 65, 90 do charset[#charset + 1] = string.char(a) end
for a = 97, 122 do charset[#charset + 1] = string.char(a) end

kitty_env.crypt.random = function(size)
    size = tonumber(size) or error("Expected number within range (0, 1024), got: " .. tostring(size), 0)
    if size < 0 or size > 1024 then
        error("Expected number within range (0, 1024), got: " .. tostring(size), 0)
    end
    local t = table.create(size)
    for i = 1, size do t[i] = charset[math.random(#charset)] end
    return table.concat(t)
end

kitty_env.crypt.generatekey = function(len)
    return kitty_env.crypt.generatebytes(len or 32)
end

kitty_env.crypt.generatebytes = function(size)
    assert(type(size) == 'number', "invalid argument #1 to 'crypt.generatebytes' (number expected, got " .. type(size) .. ')', 2)
    return base64encode(kitty_env.crypt.random(size))
end

-- aes encryption
local aes = {}
do
    local sb = { 99,124,119,123,242,107,111,197,48,1,103,43,254,215,171,118,202,130,201,125,250,89,71,240,173,212,162,175,156,164,114,192,183,253,147,38,54,63,247,204,52,165,229,241,113,216,49,21,4,199,35,195,24,150,5,154,7,18,128,226,235,39,178,117,9,131,44,26,27,110,90,160,82,59,214,179,41,227,47,132,83,209,0,237,32,252,177,91,106,203,190,57,74,76,88,207,208,239,170,251,67,77,51,133,69,249,2,127,80,60,159,168,81,163,64,143,146,157,56,245,188,182,218,33,16,255,243,210,205,12,19,236,95,151,68,23,196,167,126,61,100,93,25,115,96,129,79,220,34,42,144,136,70,238,184,20,222,94,11,219,224,50,58,10,73,6,36,92,194,211,172,98,145,149,228,121,231,200,55,109,141,213,78,169,108,86,244,234,101,122,174,8,186,120,37,46,28,166,180,198,232,221,116,31,75,189,139,138,112,62,181,102,72,3,246,14,97,53,87,185,134,193,29,158,225,248,152,17,105,217,142,148,155,30,135,233,206,85,40,223,140,161,137,13,191,230,66,104,65,153,45,15,176,84,187,22 }
    local isb = { 82,9,106,213,48,54,165,56,191,64,163,158,129,243,215,251,124,227,57,130,155,47,255,135,52,142,67,68,196,222,233,203,84,123,148,50,166,194,35,61,238,76,149,11,66,250,195,78,8,46,161,102,40,217,36,178,118,91,162,73,109,139,209,37,114,248,246,100,134,104,152,22,212,164,92,204,93,101,182,146,108,112,72,80,253,237,185,218,94,21,70,87,167,141,157,132,144,216,171,0,140,188,211,10,247,228,88,5,184,179,69,6,208,44,30,143,202,63,15,2,193,175,189,3,1,19,138,107,58,145,17,65,79,103,220,234,151,242,207,206,240,180,230,115,150,172,116,34,231,173,53,133,226,249,55,232,28,117,223,110,71,241,26,113,29,41,197,137,111,183,98,14,170,24,190,27,252,86,62,75,198,210,121,32,154,219,192,254,120,205,90,244,31,221,168,51,136,7,199,49,177,18,16,89,39,128,236,95,96,81,127,169,25,181,74,13,45,229,122,159,147,201,156,239,160,224,59,77,174,42,245,176,200,235,187,60,131,83,153,97,23,43,4,126,186,119,214,38,225,105,20,99,85,33,12,125 }
    local rcon = { 0,1,2,4,8,16,32,64,128,27,54,108,216,171,77,154,47,94,188,99,198,151,53,106,212,179,125,250,239,197,145,57 }

    local function xt(x)
        local i = bit32.lshift(x, 1)
        return bit32.band(x, 128) == 0 and i or bit32.bxor(i, 27) % 256
    end

    local function sub(s, inv)
        local m = inv and isb or sb
        for i = 1, 4 do for j = 1, 4 do s[i][j] = m[s[i][j] + 1] end end
    end

    local function shift(s, inv)
        if inv then
            s[1][3], s[2][3], s[3][3], s[4][3] = s[3][3], s[4][3], s[1][3], s[2][3]
            s[1][2], s[2][2], s[3][2], s[4][2] = s[4][2], s[1][2], s[2][2], s[3][2]
            s[1][4], s[2][4], s[3][4], s[4][4] = s[2][4], s[3][4], s[4][4], s[1][4]
        else
            s[1][3], s[2][3], s[3][3], s[4][3] = s[3][3], s[4][3], s[1][3], s[2][3]
            s[1][2], s[2][2], s[3][2], s[4][2] = s[2][2], s[3][2], s[4][2], s[1][2]
            s[1][4], s[2][4], s[3][4], s[4][4] = s[4][4], s[1][4], s[2][4], s[3][4]
        end
    end

    local function ark(s, k)
        for i = 1, 4 do for j = 1, 4 do s[i][j] = bit32.bxor(s[i][j], k[i][j]) end end
    end

    local function mix(s, inv)
        local t, u
        if inv then
            for i = 1, 4 do
                t = xt(xt(bit32.bxor(s[i][1], s[i][3])))
                u = xt(xt(bit32.bxor(s[i][2], s[i][4])))
                s[i][1], s[i][2] = bit32.bxor(s[i][1], t), bit32.bxor(s[i][2], u)
                s[i][3], s[i][4] = bit32.bxor(s[i][3], t), bit32.bxor(s[i][4], u)
            end
        end
        for j = 1, 4 do
            local i = s[j]
            t, u = bit32.bxor(i[1], i[2], i[3], i[4]), i[1]
            for k = 1, 4 do
                i[k] = bit32.bxor(i[k], t, xt(bit32.bxor(i[k], i[k + 1] or u)))
            end
        end
    end

    local function b2m(t, c, inv)
        if inv then
            table.move(c[1], 1, 4, 1, t); table.move(c[2], 1, 4, 5, t)
            table.move(c[3], 1, 4, 9, t); table.move(c[4], 1, 4, 13, t)
        else
            for i = 1, #c / 4 do
                table.clear(t[i]); table.move(c, i * 4 - 3, i * 4, 1, t[i])
            end
        end
        return t
    end

    local function xb(t, a, b)
        table.clear(t)
        for i = 1, math.min(#a, #b) do table.insert(t, bit32.bxor(a[i], b[i])) end
        return t
    end

    local function expand(key)
        local kc = b2m(if #key == 16 then {{},{},{},{}}
            elseif #key == 24 then {{},{},{},{},{},{}}
            else {{},{},{},{},{},{},{},{}}, key)
        local is, i, t, w = #key / 4, 2, {}, nil
        while #kc < (#key / 4 + 7) * 4 do
            w = table.clone(kc[#kc])
            if #kc % is == 0 then
                table.insert(w, table.remove(w, 1))
                for j = 1, 4 do w[j] = sb[w[j] + 1] end
                w[1] = bit32.bxor(w[1], rcon[i]); i += 1
            elseif #key == 32 and #kc % is == 4 then
                for j = 1, 4 do w[j] = sb[w[j] + 1] end
            end
            table.clear(t)
            xb(w, table.move(w, 1, 4, 1, t), kc[#kc - is + 1])
            table.insert(kc, w)
        end
        table.clear(t)
        for i = 1, #kc / 4 do
            table.insert(t, {}); table.move(kc, i * 4 - 3, i * 4, 1, t[#t])
        end
        return t
    end

    local function enc(key, km, pt, ps, r)
        b2m(ps, pt); ark(ps, km[1])
        for i = 2, #key / 4 + 6 do sub(ps); shift(ps); mix(ps); ark(ps, km[i]) end
        sub(ps); shift(ps); ark(ps, km[#km])
        return b2m(r, ps, true)
    end

    local function dec(key, km, ct, cs, r)
        b2m(cs, ct); ark(cs, km[#km]); shift(cs, true); sub(cs, true)
        for i = #key / 4 + 6, 2, -1 do ark(cs, km[i]); mix(cs, true); shift(cs, true); sub(cs, true) end
        ark(cs, km[1])
        return b2m(r, cs, true)
    end

    local function tobytes(a)
        if type(a) == "string" then
            local r = {}
            for i = 1, #a, 7997 do table.move({string.byte(a, i, i + 7996)}, 1, 7997, i, r) end
            return r
        elseif type(a) == "table" then
            for _, i in ipairs(a) do
                assert(type(i) == "number" and math.floor(i) == i and 0 <= i and i < 256, "unable to cast value to bytes")
            end
            return a
        end
        error("unable to cast value to bytes", 0)
    end

    local function tostring_(t)
        local parts = {}
        for i = 1, #t, 7997 do parts[#parts + 1] = string.char(table.unpack(t, i, math.min(i + 7996, #t))) end
        return table.concat(parts)
    end

    function aes.encrypt_cbc(key, pt, iv)
        key, pt = tobytes(key), tobytes(pt)
        assert(#key == 16 or #key == 24 or #key == 32, "key must be either 16, 24 or 32 bytes long")
        iv = iv and tobytes(iv) or table.create(16, 0)
        assert(#iv == 16, "initialization vector must be 16 bytes long")
        local km, b, k, p, s, t = expand(key), {}, {}, iv, {{},{},{},{}}, {}
        for i = 1, #pt, 16 do
            table.move(pt, i, i + 15, 1, k)
            table.move(enc(key, km, xb(t, k, p), s, p), 1, 16, i, b)
        end
        return b
    end

    function aes.decrypt_cbc(key, ct, iv)
        key, ct = tobytes(key), tobytes(ct)
        assert(#key == 16 or #key == 24 or #key == 32, "key must be either 16, 24 or 32 bytes long")
        assert(#ct % 16 == 0, "input must be a multiple of 16 bytes in length")
        iv = iv and tobytes(iv) or table.create(16, 0)
        assert(#iv == 16, "initialization vector must be 16 bytes long")
        local km, b, k, p, s, t = expand(key), {}, {}, iv, {{},{},{},{}}, {}
        for i = 1, #ct, 16 do
            table.move(ct, i, i + 15, 1, k)
            table.move(xb(k, dec(key, km, k, s, t), p), 1, 16, i, b)
            table.move(ct, i, i + 15, 1, p)
        end
        return b
    end

    aes.to_string = tostring_
    aes.to_bytes = tobytes
end

local function normiv(iv)
    if iv == nil then
        local b = table.create(16)
        for i = 1, 16 do b[i] = string.char(math.random(0, 255)) end
        return base64encode(table.concat(b))
    end
    local bytes = aes.to_bytes(base64decode(iv))
    if #bytes == 16 then return iv end
    if #bytes > 16 then
        local cut = table.create(16)
        for i = 1, 16 do cut[i] = bytes[i] end
        return base64encode(aes.to_string(cut))
    end
    while #bytes < 16 do bytes[#bytes + 1] = 0 end
    return base64encode(aes.to_string(bytes))
end

crypt.encrypt = function(data, key, iv, mode)
    mode = string.lower(mode or 'cbc')
    assert(mode == 'cbc', "unsupported crypt.encrypt mode: " .. tostring(mode), 0)
    
    key = key or crypt.generatekey(32)
    iv = normiv(iv)
    local kb = aes.to_bytes(base64decode(key))
    assert(#kb == 16 or #kb == 24 or #kb == 32, "key must be either 16, 24 or 32 bytes long")
    local ib = aes.to_bytes(base64decode(iv))
    assert(#ib == 16, "initialization vector must be 16 bytes long")
    local pt = aes.to_bytes(tostring(data))
    local pad = 16 - (#pt % 16)
    for _ = 1, pad do pt[#pt + 1] = pad end
    return base64encode(aes.to_string(aes.encrypt_cbc(kb, pt, ib))), iv
end

crypt.decrypt = function(data, key, iv, mode)
    assert(type(data) == 'string', "invalid argument #1 to 'crypt.decrypt' (string expected)", 2)
    
    mode = string.lower(mode or 'cbc')
    assert(mode == 'cbc', "unsupported crypt.decrypt mode: " .. tostring(mode), 0)
    
    key = key or crypt.generatekey(32)
    iv = normiv(iv)
    local kb = aes.to_bytes(base64decode(key))
    assert(#kb == 16 or #kb == 24 or #kb == 32, "key must be either 16, 24 or 32 bytes long")
    local ib = aes.to_bytes(base64decode(iv))
    assert(#ib == 16, "initialization vector must be 16 bytes long")
    local pt = aes.decrypt_cbc(kb, aes.to_bytes(base64decode(data)), ib)
    local pad = pt[#pt]
    if pad and pad >= 1 and pad <= 16 and #pt >= pad then
        local ok = true
        for i = #pt - pad + 1, #pt do
            if pt[i] ~= pad then ok = false; break end
        end
        if ok then for _ = 1, pad do table.remove(pt) end end
    end
    return aes.to_string(pt)
end

crypt.hash = function(data, name)
    assert(type(data) == 'string', "invalid argument #1 to 'crypt.hash' (string expected, got " .. type(data) .. ')', 2)
    assert(type(name) == 'string', "invalid argument #2 to 'crypt.hash' (string expected, got " .. type(name) .. ')', 2)

    local status, response = sendData("/hash?p=" .. client_pid .. "&alg=" .. name, base64encode(data))
    if status ~= 200 or response == 'fail' or response == nil or response == '' then
        return nil
    end
    return response
end

crypt.hmac = function(key, data, alg)
    assert(type(key) == 'string', "invalid argument #1 to 'crypt.hmac' (string expected, got " .. type(key) .. ')', 2)
    assert(type(data) == 'string', "invalid argument #2 to 'crypt.hmac' (string expected, got " .. type(data) .. ')', 2)

    local body = base64encode(key) .. "\n" .. base64encode(data)
    local status, response = sendData("/hmac?p=" .. client_pid .. "&alg=" .. (alg or 'sha256'), body)
    if status ~= 200 or response == 'fail' or response == nil or response == '' then
        return nil
    end
    return response
end
-- end

-- closures (loadstring fixed by @byteblue but return value in loadstring is not supported properly, we actively working on that)
local function sendPointer(method_name, script)
	local holder = Instance.new('ObjectValue')
	holder.Name = http_service:GenerateGUID(false)
	holder.Value = script
	holder.Parent = container

	local status, response = sendData("/" .. method_name .. "?p=".. client_pid .. "&n=" .. holder.Name, "")
	holder:Destroy()
    if status ~= 200 and typeof(response) ~= "string" then
        return nil
    end
	return response
end

kitty_env.getscriptbytecode = function(script)
    assert(typeof(script) == 'Instance' and script:IsA('LuaSourceContainer'), "invalid argument #1 to 'getscriptbytecode' (LuaSourceContainer expected, got " .. typeof(script) .. ')', 2)
	local response = sendPointer("getscriptbytecode", script)
	if response == nil then
		return nil
	end
	return base64decode(response)
end
kitty_env.dumpstring = kitty_env.getscriptbytecode

-- from skidsploit getnamecallmethod realisation
local namecallCache = {}

local function generateProbe(object)
    local _, err = xpcall(function()
        object:__namecall()
    end, function()
        return debug.info(2, "f")
    end)
    return err
end

local function extractMethod(err)
    if type(err) ~= "string" then
        return nil
    end
    return err:match("^(.+) is not a valid member of %w+$")
end

local firstProbe = generateProbe(OverlapParams.new())
local secondProbe = generateProbe(Color3.new())

kitty_env.getnamecallmethod = function()
    local _, err = pcall(firstProbe)
    local method = extractMethod(err)
    if not method then
        _, err = pcall(secondProbe)
        method = extractMethod(err)
    end

    local fixerData = newproxy(true)
    local fixerMeta = getmetatable(fixerData)
    fixerMeta.__namecall = function()
        local _, probeError = pcall(firstProbe)
        local probeMethod = extractMethod(probeError)
        if not probeMethod then
            _, probeError = pcall(secondProbe)
            probeMethod = extractMethod(probeError)
        end
    end
    fixerData:__namecall()

    if not method or method == "__namecall" then
        local cached = namecallCache[coroutine.running()]
        if cached then
            return cached
        end
        return nil
    end

    namecallCache[coroutine.running()] = method
    return method
end
-- end

--[[kitty_env.loadstring = function(source, chunkName)
    chunkName = chunkName or "loadstring"
    local clock = tick()
    
    while tick() - clock < 5 do
        local module_name = "Loadstring:"..tostring(math.random(1, 1e9))
        local module = copyModule(module_name)

        local status = sendData("/loadstring?c=" .. module_name .. "&p=" .. client_pid .. "&cn=" .. chunkName, source)

        if status == 200 then
            local success, exec = pcall(require, module)
            
            if success and type(exec) == "function" then
                local env = setmetatable({
                    script = module,
                    print = function(...)
                        local args = table.pack(...)
                        if args.n == 0 then return end
                        local has_content = false
                        for i = 1, args.n do
                            if args[i] ~= nil and tostring(args[i]) ~= "" then
                                has_content = true
                                break
                            end
                        end
                        if has_content then
                            kitty_env.print(...)
                        end
                    end
                }, { __index = kitty_env })
                
                pcall(setfenv, exec, env)
                
                return function(...)
                    local results = table.pack(pcall(exec, ...))
                    
                    if module then 
                        pcall(function() module:Destroy() end) 
                    end
                    
                    if not results[1] then
                        error(results[2], 0)
                    end
                    
                    local inner_func = results[2]
                    
                    if type(inner_func) == "function" then
                        local final_results = table.pack(pcall(inner_func, ...))
                        if not final_results[1] then
                            error(final_results[2], 0)
                        end
                        if final_results.n > 1 then
                            return table.unpack(final_results, 2, final_results.n)
                        end
                        return
                    end
                    
                    if results.n > 1 then
                        return table.unpack(results, 2, results.n)
                    end
                    return
                end
            else
                if module then pcall(function() module:Destroy() end) end
                return nil, "loadstring failed: " .. tostring(exec)
            end
        elseif status == 204 then
            if module then pcall(function() module:Destroy() end) end
        else
            if module then pcall(function() module:Destroy() end) end
        end
        
        task.wait(0.1)
    end
    
    return nil, "loadstring failed and timed out"
end]]
-- end

-- virtual filesystem
local virtual_fs_files = {}
local virtual_fs_folders = {
	[""] = true
}

local blocked_extensions = {".exe", ".scr", ".bat", ".com", ".csh", ".msi", ".vb", ".vbs",
	".vbe", ".ws", ".wsf", ".wsh", ".ps1", ".py", ".apk", ".pif", ".cpl", ".msc",
	".jar", ".cmd", ".hta", ".gadget", ".inf", ".ins", ".isp", ".psd1", ".psm1",
	".reg", ".scf", ".shb", ".sys", ".js", ".jse", ".lnk", ".msp",
	".zip", ".rar", ".7z", ".tar", ".gz", ".cab", ".iso", ".img",
	".dll", ".ocx", ".drv", ".vxd", ".xml", ".ini", ".cpp", ".c", ".url", ".uri",
	".deb", ".rpm", ".sh", ".bash", ".zsh", ".fish", ".npm"}

local function isBlockedExtension(path: string): boolean
	local name = (path:lower():gsub("[%s.]+$", ""))
	for _, ext in blocked_extensions do
		if name:sub(-#ext) == ext then
			return true
		end
	end
	return false
end

local function normalizePath(path)
	path = path:gsub("\\", "/")
	local parts = {}
	for part in path:gmatch("[^/]+") do
		if part == ".." then
			if #parts > 0 then table.remove(parts) end
		elseif part ~= "." and part ~= "" then
			table.insert(parts, part)
		end
	end
	return table.concat(parts, "/")
end

local function ensureFolder(path)
	if path == "" then return end
	local parts = string.split(path, "/")
	local current = ""
	for _, part in ipairs(parts) do
		current = current == "" and part or (current .. "/" .. part)
		virtual_fs_folders[current] = true
	end
end

local function isPathSafe(rawPath)
	if rawPath:match("^%a:[/\\]") or rawPath:match("^[/\\]") then
		return false
	end
	if rawPath:match("^%.%.") or rawPath:match("[/\\]%.%.") then
		return false
	end
	return true
end

kitty_env.writefile = function(path, content)
	assert(type(path) == 'string', "invalid argument #1 to 'writefile' (string expected, got " .. type(path) .. ')', 2)
	if isBlockedExtension(path) then
		error('writefile error: extension is blocked: ' .. path, 2)
	end
	if not isPathSafe(path) then
		error('writefile error: cannot write to illegal location: ' .. path, 2)
	end
	local norm = normalizePath(path)
	local dir = string.match(norm, "^(.*)/") or ""
	if dir ~= "" then ensureFolder(dir) end
	virtual_fs_files[norm] = tostring(content)
end

kitty_env.appendfile = function(path, content)
	assert(type(path) == 'string', "invalid argument #1 to 'appendfile' (string expected, got " .. type(path) .. ')', 2)
	if isBlockedExtension(path) then
		error('appendfile error: extension is blocked: ' .. path, 2)
	end
	if not isPathSafe(path) then
		error('appendfile error: cannot write to illegal location: ' .. path, 2)
	end
	local norm = normalizePath(path)
	local dir = string.match(norm, "^(.*)/") or ""
	if dir ~= "" then ensureFolder(dir) end
	virtual_fs_files[norm] = (virtual_fs_files[norm] or "") .. tostring(content)
end

kitty_env.readfile = function(path)
	assert(type(path) == 'string', "invalid argument #1 to 'readfile' (string expected, got " .. type(path) .. ')', 2)
	local norm = normalizePath(path)
	if virtual_fs_files[norm] == nil then
		error('readfile error: cannot read: ' .. path, 2)
	end
	return virtual_fs_files[norm]
end

kitty_env.isfile = function(path)
	assert(type(path) == 'string', "invalid argument #1 to 'isfile' (string expected, got " .. type(path) .. ')', 2)
	return virtual_fs_files[normalizePath(path)] ~= nil
end

kitty_env.isfolder = function(path)
	assert(type(path) == 'string', "invalid argument #1 to 'isfolder' (string expected, got " .. type(path) .. ')', 2)
	return virtual_fs_folders[normalizePath(path)] ~= nil
end

kitty_env.delfile = function(path)
	assert(type(path) == 'string', "invalid argument #1 to 'delfile' (string expected, got " .. type(path) .. ')', 2)
	if not isPathSafe(path) then
		error('delfile error: cannot delete illegal location: ' .. path, 2)
	end
	local norm = normalizePath(path)
	if virtual_fs_files[norm] == nil then
		error('delfile error: cannot delete: ' .. path, 2)
	end
	virtual_fs_files[norm] = nil
end

kitty_env.delfolder = function(path)
	assert(type(path) == 'string', "invalid argument #1 to 'delfolder' (string expected, got " .. type(path) .. ')', 2)
	if not isPathSafe(path) then
		error('delfolder error: cannot delete illegal location: ' .. path, 2)
	end
	local norm = normalizePath(path)
	if norm == "" or not virtual_fs_folders[norm] then
		error('delfolder error: cannot delete: ' .. path, 2)
	end
	
	local prefix = norm .. "/"
	
	for k in pairs(virtual_fs_files) do
		if k:sub(1, #prefix) == prefix or k == norm then
			virtual_fs_files[k] = nil
		end
	end
	
	for k in pairs(virtual_fs_folders) do
		if k:sub(1, #prefix) == prefix or k == norm then
			virtual_fs_folders[k] = nil
		end
	end
end

kitty_env.makefolder = function(path)
	assert(type(path) == 'string', "invalid argument #1 to 'makefolder' (string expected, got " .. type(path) .. ')', 2)
	if not isPathSafe(path) then
		error('makefolder error: cannot make illegal location: ' .. path, 2)
	end
	local norm = normalizePath(path)
	if norm == "" then return end
	ensureFolder(norm)
end

kitty_env.listfiles = function(path)
    assert(type(path) == 'string' or type(path) == 'nil', "invalid argument #1 to 'listfiles' (string expected, got " .. type(path) .. ')', 2)
    local norm = normalizePath(path or "")
    
    if not virtual_fs_folders[norm] then
        error('listfiles error: folder not found: ' .. tostring(path), 2)
    end
    
    local results = {}
    local prefix = norm == "" and "" or (norm .. "/")
    
    for k in pairs(virtual_fs_files) do
        if k:sub(1, #prefix) == prefix and k ~= norm then
            local display_path = k:gsub("/", "\\")
            table.insert(results, display_path)
        end
    end
    
    for k in pairs(virtual_fs_folders) do
        if k ~= norm and k:sub(1, #prefix) == prefix then
            local display_path = k:gsub("/", "\\")
            table.insert(results, display_path)
        end
    end
    
    return results
end
-- end

-- request, http_request, http.request
kitty_env.request = function(options)
    if type(options) ~= "table" then
        return {
            Success = false, Body = '', StatusCode = 0,
            StatusMessage = "request expects a table of options", Headers = {},
        }
    end
    if type(options.Url) ~= "string" then
        return {
            Success = false, Body = '', StatusCode = 0,
            StatusMessage = "invalid Url", Headers = {},
        }
    end

    is_url_allowed(options.Url)

    options.Headers = options.Headers or {}

    local game_id = tostring(game.GameId or "")
    local place_id = tostring(game.PlaceId or "")
    
    local session_id_json = http_service:JSONEncode({
        GameId = game_id,
        PlaceId = place_id
    })

    local function has_header(key)
        for k in pairs(options.Headers) do
            if k:lower() == key:lower() then return true end
        end
        return false
    end

    if not has_header("Roblox-Session-Id") then
        options.Headers["Roblox-Session-Id"] = session_id_json
    end
    if not has_header("Roblox-Place-Id") then
        options.Headers["Roblox-Place-Id"] = place_id
    end
    if not has_header("Roblox-Game-Id") then
        options.Headers["Roblox-Game-Id"] = game_id
    end
    if not has_header("Accept") then
        options.Headers["Accept"] = "*/*"
    end

    local okEncode, encoded = pcall(function()
        return http_service:JSONEncode(options)
    end)
    if not okEncode or type(encoded) ~= "string" then
        return {
            Success = false, Body = '', StatusCode = 0,
            StatusMessage = "Request could not encode the options", Headers = {},
        }
    end

    local status, body = sendJsonData('/request?p=' .. client_pid, encoded)
    if status ~= 200 then
        return {
            Success = false, Body = '', StatusCode = status or 0,
            StatusMessage = 'Request failed (status ' .. tostring(status) .. ')',
            Headers = {},
        }
    end
    if type(body) ~= "string" or body == '' then
        return {
            Success = false, Body = '', StatusCode = 0,
            StatusMessage = "Request failed (empty response)", Headers = {},
        }
    end

    local okDecode, decoded = pcall(function()
        return http_service:JSONDecode(body)
    end)
    if not okDecode or type(decoded) ~= "table" then
        return {
            Success = false, Body = '', StatusCode = 0,
            StatusMessage = "request failed (invalid JSON response)", Headers = {},
        }
    end

    return decoded
end
kitty_env.http_request = kitty_env.request
kitty_env.http = {request = kitty_env.request}
kitty_env.syn = {request = kitty_env.request}
-- end

-- signal library
kitty_env.Signal = {}

function kitty_env.Signal.new()
	local handlers = {}
	local waiting = {}

	local signal = {}

	local function fire(...)
		local snapshot = table.clone(handlers)
		for _, entry in ipairs(snapshot) do
			task.spawn(entry.fn, ...)
		end
		local threads = waiting
		waiting = {}
		for _, thread in ipairs(threads) do
			task.spawn(thread, ...)
		end
	end

	function signal:Connect(fn)
		assert(type(fn) == 'function', "invalid argument #1 to 'Connect' (function expected, got " .. type(fn) .. ')', 2)
		local entry = {fn = fn}
		table.insert(handlers, entry)

		local connection = {}
		function connection:Disconnect()
			local idx = table.find(handlers, entry)
			if idx then
				table.remove(handlers, idx)
			end
		end

		return connection
	end

	function signal:Once(fn)
		assert(type(fn) == 'function', "invalid argument #1 to 'Once' (function expected, got " .. type(fn) .. ')', 2)
		local connection
		connection = self:Connect(function(...)
			connection:Disconnect()
			fn(...)
		end)
		return connection
	end

	function signal:Wait()
		local thread = coroutine.running()
		table.insert(waiting, thread)
		return coroutine.yield()
	end

	function signal:Fire(...)
		fire(...)
	end

	return signal
end
-- end

-- stats proxier (for myriad vuln test)
kitty_env.Stats = function()
    return proxiedServices.Stats.proxy
end
-- end

-- drawing
local DrawingCache = {}
local DrawingObjects = {}

local Fonts = {
    [0] = Enum.Font.Arial,
    [1] = Enum.Font.BuilderSans,
    [2] = Enum.Font.Gotham,
    [3] = Enum.Font.RobotoMono
}

local UI = Instance.new("ScreenGui")
UI.Name = "DrawingLib"
UI.DisplayOrder = 999999
UI.IgnoreGuiInset = true
UI.ResetOnSpawn = false
UI.Parent = core_gui

local Drawing = {
    Fonts = {
        UI = 0,
        System = 1,
        Plex = 2,
        Monospace = 3
    }
}

Drawing.new = function(drawingType)
    if type(drawingType) ~= "string" then
        error("Drawing.new: argument must be a string, got " .. type(drawingType), 2)
    end

    local validTypes = {
        Line = true,
        Square = true,
        Rectangle = true,
        Circle = true,
        Text = true,
        Image = true,
        Triangle = true,
        Quad = true
    }
    
    if not validTypes[drawingType] then
        error("Drawing.new: invalid drawing type '" .. drawingType .. "'", 2)
    end

    local drawingObj = Instance.new("Frame")
    drawingObj.BackgroundTransparency = 1
    drawingObj.BorderSizePixel = 0
    drawingObj.Size = UDim2.fromOffset(0, 0)
    drawingObj.Position = UDim2.fromOffset(0, 0)
    drawingObj.Parent = UI
    
    local properties = {
        Visible = true,
        Color = Color3.new(1, 1, 1),
        Transparency = 1,
        ZIndex = 1
    }
    
    local self = newproxy(true)
    local mt = getmetatable(self)
    
    local removed = false
    
    mt.__index = function(_, key)
        if key == "__OBJECT_EXISTS" then
            return not removed
        end
        return properties[key]
    end
    
    mt.__newindex = function(_, key, value)
        if removed then return end
        
        if key == "__OBJECT_EXISTS" then
            return
        end
        
        properties[key] = value
        
        if key == "Visible" then
            drawingObj.Visible = value
        elseif key == "Color" then
            drawingObj.BackgroundColor3 = value
        elseif key == "Transparency" then
            drawingObj.BackgroundTransparency = 1 - value
        elseif key == "ZIndex" then
            drawingObj.ZIndex = value
        end
    end
    
    mt.__tostring = function()
        return "Drawing"
    end
    
    mt.__metatable = "The metatable is locked"
    
    properties.Remove = function()
        if removed then return end
        
        removed = true
        
        if drawingObj and drawingObj.Parent then
            drawingObj:Destroy()
        end
        
        for i, obj in ipairs(DrawingCache) do
            if obj.proxy == self then
                table.remove(DrawingCache, i)
                break
            end
        end
        
        DrawingObjects[self] = nil
    end
    
    properties.Destroy = properties.Remove
    
    table.insert(DrawingCache, {proxy = self, instance = drawingObj})
    DrawingObjects[self] = true
    
    if drawingType == "Line" then
        drawingObj.AnchorPoint = Vector2.new(0.5, 0.5)
        
        properties.From = Vector2.zero
        properties.To = Vector2.zero
        properties.Thickness = 1
        
        local updateLine = function()
            if removed then return end
            
            local from = properties.From
            local to = properties.To
            local dx = to.X - from.X
            local dy = to.Y - from.Y
            local length = math.sqrt(dx * dx + dy * dy)
            
            drawingObj.Size = UDim2.fromOffset(length, properties.Thickness)
            drawingObj.Position = UDim2.fromOffset((from.X + to.X) / 2, (from.Y + to.Y) / 2)
            drawingObj.Rotation = math.deg(math.atan2(dy, dx))
            drawingObj.BackgroundTransparency = 1 - properties.Transparency
            drawingObj.BackgroundColor3 = properties.Color
        end
        
        local oldNewindex = mt.__newindex
        mt.__newindex = function(t, k, v)
            oldNewindex(t, k, v)
            if k == "From" or k == "To" or k == "Thickness" then
                updateLine()
            end
        end
        
    elseif drawingType == "Square" or drawingType == "Rectangle" then
        local stroke = Instance.new("UIStroke")
        stroke.Parent = drawingObj
        stroke.Thickness = 1
        stroke.Color = Color3.new(1, 1, 1)
        
        properties.Size = Vector2.zero
        properties.Position = Vector2.zero
        properties.Filled = false
        properties.Thickness = 1
        
        local updateSquare = function()
            if removed then return end
            
            drawingObj.Size = UDim2.fromOffset(properties.Size.X, properties.Size.Y)
            drawingObj.Position = UDim2.fromOffset(properties.Position.X, properties.Position.Y)
            
            if properties.Filled then
                drawingObj.BackgroundTransparency = 1 - properties.Transparency
                drawingObj.BackgroundColor3 = properties.Color
                stroke.Enabled = false
            else
                drawingObj.BackgroundTransparency = 1
                stroke.Enabled = true
                stroke.Color = properties.Color
                stroke.Thickness = properties.Thickness
                stroke.Transparency = 1 - properties.Transparency
            end
        end
        
        local oldNewindex = mt.__newindex
        mt.__newindex = function(t, k, v)
            oldNewindex(t, k, v)
            if k == "Size" or k == "Position" or k == "Filled" or k == "Thickness" or k == "Color" or k == "Transparency" then
                updateSquare()
            end
        end
        
    elseif drawingType == "Circle" then
        local stroke = Instance.new("UIStroke")
        stroke.Parent = drawingObj
        stroke.Thickness = 1
        
        local corner = Instance.new("UICorner")
        corner.CornerRadius = UDim.new(1, 0)
        corner.Parent = drawingObj
        
        properties.Radius = 50
        properties.Position = Vector2.zero
        properties.Filled = false
        properties.Thickness = 1
        properties.NumSides = 64
        
        local updateCircle = function()
            if removed then return end
            
            local diameter = properties.Radius * 2
            drawingObj.Size = UDim2.fromOffset(diameter, diameter)
            drawingObj.Position = UDim2.fromOffset(properties.Position.X - properties.Radius, properties.Position.Y - properties.Radius)
            
            if properties.Filled then
                drawingObj.BackgroundTransparency = 1 - properties.Transparency
                drawingObj.BackgroundColor3 = properties.Color
                stroke.Enabled = false
            else
                drawingObj.BackgroundTransparency = 1
                stroke.Enabled = true
                stroke.Color = properties.Color
                stroke.Thickness = properties.Thickness
                stroke.Transparency = 1 - properties.Transparency
            end
        end
        
        local oldNewindex = mt.__newindex
        mt.__newindex = function(t, k, v)
            oldNewindex(t, k, v)
            if k == "Radius" or k == "Position" or k == "Filled" or k == "Thickness" or k == "Color" or k == "Transparency" then
                updateCircle()
            end
        end
        
    elseif drawingType == "Triangle" then
        drawingObj:Destroy()
        
        drawingObj = Instance.new("Frame")
        drawingObj.BackgroundTransparency = 1
        drawingObj.BorderSizePixel = 0
        drawingObj.Size = UDim2.fromOffset(100, 100)
        drawingObj.Parent = UI
        
        local point1Frame = Instance.new("Frame")
        point1Frame.AnchorPoint = Vector2.new(0.5, 0.5)
        point1Frame.BackgroundColor3 = Color3.new(1, 1, 1)
        point1Frame.BorderSizePixel = 0
        point1Frame.Parent = drawingObj
        
        local point2Frame = Instance.new("Frame")
        point2Frame.AnchorPoint = Vector2.new(0.5, 0.5)
        point2Frame.BackgroundColor3 = Color3.new(1, 1, 1)
        point2Frame.BorderSizePixel = 0
        point2Frame.Parent = drawingObj
        
        local point3Frame = Instance.new("Frame")
        point3Frame.AnchorPoint = Vector2.new(0.5, 0.5)
        point3Frame.BackgroundColor3 = Color3.new(1, 1, 1)
        point3Frame.BorderSizePixel = 0
        point3Frame.Parent = drawingObj
        
        properties.PointA = Vector2.zero
        properties.PointB = Vector2.new(50, 100)
        properties.PointC = Vector2.new(100, 0)
        properties.Filled = false
        properties.Thickness = 1
        
        local updateTriangle = function()
            if removed then return end
            
            local pA = properties.PointA
            local pB = properties.PointB
            local pC = properties.PointC
            
            local minX = math.min(pA.X, pB.X, pC.X)
            local minY = math.min(pA.Y, pB.Y, pC.Y)
            local maxX = math.max(pA.X, pB.X, pC.X)
            local maxY = math.max(pA.Y, pB.Y, pC.Y)
            
            drawingObj.Position = UDim2.fromOffset(minX, minY)
            drawingObj.Size = UDim2.fromOffset(maxX - minX, maxY - minY)
            
            local function drawLine(frame, p1, p2)
                local dx = p2.X - p1.X
                local dy = p2.Y - p1.Y
                local length = math.sqrt(dx * dx + dy * dy)
                
                frame.Size = UDim2.fromOffset(length, properties.Thickness)
                frame.Position = UDim2.fromOffset((p1.X + p2.X) / 2 - minX, (p1.Y + p2.Y) / 2 - minY)
                frame.Rotation = math.deg(math.atan2(dy, dx))
                frame.BackgroundColor3 = properties.Color
                frame.BackgroundTransparency = 1 - properties.Transparency
            end
            
            if properties.Filled then
                point1Frame.Visible = false
                point2Frame.Visible = false
                point3Frame.Visible = false
                drawingObj.BackgroundColor3 = properties.Color
                drawingObj.BackgroundTransparency = 1 - properties.Transparency
            else
                point1Frame.Visible = true
                point2Frame.Visible = true
                point3Frame.Visible = true
                drawingObj.BackgroundTransparency = 1
                
                drawLine(point1Frame, pA, pB)
                drawLine(point2Frame, pB, pC)
                drawLine(point3Frame, pC, pA)
            end
        end
        
        local oldNewindex = mt.__newindex
        mt.__newindex = function(t, k, v)
            oldNewindex(t, k, v)
            if k == "PointA" or k == "PointB" or k == "PointC" or k == "Filled" or k == "Thickness" or k == "Color" or k == "Transparency" then
                updateTriangle()
            end
        end
        
    elseif drawingType == "Quad" then
        drawingObj:Destroy()
        
        drawingObj = Instance.new("Frame")
        drawingObj.BackgroundTransparency = 1
        drawingObj.BorderSizePixel = 0
        drawingObj.Size = UDim2.fromOffset(100, 100)
        drawingObj.Parent = UI
        
        local line1 = Instance.new("Frame")
        line1.AnchorPoint = Vector2.new(0.5, 0.5)
        line1.BackgroundColor3 = Color3.new(1, 1, 1)
        line1.BorderSizePixel = 0
        line1.Parent = drawingObj
        
        local line2 = Instance.new("Frame")
        line2.AnchorPoint = Vector2.new(0.5, 0.5)
        line2.BackgroundColor3 = Color3.new(1, 1, 1)
        line2.BorderSizePixel = 0
        line2.Parent = drawingObj
        
        local line3 = Instance.new("Frame")
        line3.AnchorPoint = Vector2.new(0.5, 0.5)
        line3.BackgroundColor3 = Color3.new(1, 1, 1)
        line3.BorderSizePixel = 0
        line3.Parent = drawingObj
        
        local line4 = Instance.new("Frame")
        line4.AnchorPoint = Vector2.new(0.5, 0.5)
        line4.BackgroundColor3 = Color3.new(1, 1, 1)
        line4.BorderSizePixel = 0
        line4.Parent = drawingObj
        
        properties.PointA = Vector2.zero
        properties.PointB = Vector2.new(100, 0)
        properties.PointC = Vector2.new(100, 100)
        properties.PointD = Vector2.new(0, 100)
        properties.Filled = false
        properties.Thickness = 1
        
        local updateQuad = function()
            if removed then return end
            
            local pA = properties.PointA
            local pB = properties.PointB
            local pC = properties.PointC
            local pD = properties.PointD
            
            local minX = math.min(pA.X, pB.X, pC.X, pD.X)
            local minY = math.min(pA.Y, pB.Y, pC.Y, pD.Y)
            local maxX = math.max(pA.X, pB.X, pC.X, pD.X)
            local maxY = math.max(pA.Y, pB.Y, pC.Y, pD.Y)
            
            drawingObj.Position = UDim2.fromOffset(minX, minY)
            drawingObj.Size = UDim2.fromOffset(maxX - minX, maxY - minY)
            
            local function drawLine(frame, p1, p2)
                local dx = p2.X - p1.X
                local dy = p2.Y - p1.Y
                local length = math.sqrt(dx * dx + dy * dy)
                
                frame.Size = UDim2.fromOffset(length, properties.Thickness)
                frame.Position = UDim2.fromOffset((p1.X + p2.X) / 2 - minX, (p1.Y + p2.Y) / 2 - minY)
                frame.Rotation = math.deg(math.atan2(dy, dx))
                frame.BackgroundColor3 = properties.Color
                frame.BackgroundTransparency = 1 - properties.Transparency
            end
            
            if properties.Filled then
                line1.Visible = false
                line2.Visible = false
                line3.Visible = false
                line4.Visible = false
                drawingObj.BackgroundColor3 = properties.Color
                drawingObj.BackgroundTransparency = 1 - properties.Transparency
            else
                line1.Visible = true
                line2.Visible = true
                line3.Visible = true
                line4.Visible = true
                drawingObj.BackgroundTransparency = 1
                
                drawLine(line1, pA, pB)
                drawLine(line2, pB, pC)
                drawLine(line3, pC, pD)
                drawLine(line4, pD, pA)
            end
        end
        
        local oldNewindex = mt.__newindex
        mt.__newindex = function(t, k, v)
            oldNewindex(t, k, v)
            if k == "PointA" or k == "PointB" or k == "PointC" or k == "PointD" or k == "Filled" or k == "Thickness" or k == "Color" or k == "Transparency" then
                updateQuad()
            end
        end
        
    elseif drawingType == "Text" then
        drawingObj:Destroy()
        
        drawingObj = Instance.new("TextLabel")
        drawingObj.BackgroundTransparency = 1
        drawingObj.BorderSizePixel = 0
        drawingObj.TextColor3 = Color3.new(1, 1, 1)
        drawingObj.TextSize = 13
        drawingObj.Font = Enum.Font.Code
        drawingObj.Text = ""
        drawingObj.AutomaticSize = Enum.AutomaticSize.XY
        drawingObj.Parent = UI
        
        properties.Text = ""
        properties.Size = 13
        properties.Center = false
        properties.Outline = false
        properties.OutlineColor = Color3.new(0, 0, 0)
        properties.Position = Vector2.zero
        properties.Font = 2
        properties.TextBounds = Vector2.zero
        
        local updateText = function()
            if removed then return end
            
            drawingObj.Text = tostring(properties.Text)
            drawingObj.TextSize = properties.Size
            drawingObj.Position = UDim2.fromOffset(properties.Position.X, properties.Position.Y)
            drawingObj.TextColor3 = properties.Color
            drawingObj.TextTransparency = 1 - properties.Transparency
            drawingObj.Font = Fonts[properties.Font] or Enum.Font.Code
            
            if properties.Center then
                drawingObj.TextXAlignment = Enum.TextXAlignment.Center
                drawingObj.TextYAlignment = Enum.TextYAlignment.Center
            else
                drawingObj.TextXAlignment = Enum.TextXAlignment.Left
                drawingObj.TextYAlignment = Enum.TextYAlignment.Top
            end
            
            if properties.Outline then
                drawingObj.TextStrokeTransparency = 0
                drawingObj.TextStrokeColor3 = properties.OutlineColor
            else
                drawingObj.TextStrokeTransparency = 1
            end
            
            game:GetService("RunService").RenderStepped:Wait()
            properties.TextBounds = drawingObj.TextBounds
        end
        
        local oldNewindex = mt.__newindex
        mt.__newindex = function(t, k, v)
            oldNewindex(t, k, v)
            if k ~= "TextBounds" then
                task.spawn(updateText)
            end
        end
        
    elseif drawingType == "Image" then
        drawingObj:Destroy()
        
        drawingObj = Instance.new("ImageLabel")
        drawingObj.BackgroundTransparency = 1
        drawingObj.BorderSizePixel = 0
        drawingObj.Parent = UI
        
        properties.Data = ""
        properties.Size = Vector2.zero
        properties.Position = Vector2.zero
        properties.Rounding = 0
        
        local updateImage = function()
            if removed then return end
            
            drawingObj.Image = properties.Data
            drawingObj.Size = UDim2.fromOffset(properties.Size.X, properties.Size.Y)
            drawingObj.Position = UDim2.fromOffset(properties.Position.X, properties.Position.Y)
            drawingObj.ImageTransparency = 1 - properties.Transparency
            drawingObj.ImageColor3 = properties.Color
        end
        
        local oldNewindex = mt.__newindex
        mt.__newindex = function(t, k, v)
            oldNewindex(t, k, v)
            if k == "Data" or k == "Size" or k == "Position" or k == "Transparency" or k == "Color" then
                updateImage()
            end
        end
    end
    
    return self
end

kitty_env.Drawing = Drawing

kitty_env.isrenderobj = function(obj)
    if type(obj) ~= "userdata" then
        return false
    end
    
    local success, exists = pcall(function()
        return obj.__OBJECT_EXISTS
    end)
    
    return success and exists == true
end

kitty_env.cleardrawcache = function()
    for i = #DrawingCache, 1, -1 do
        local data = DrawingCache[i]
        if data and data.proxy then
            pcall(function()
                data.proxy:Remove()
            end)
        end
    end
    
    DrawingCache = {}
    DrawingObjects = {}
end

kitty_env.getrenderproperty = function(obj, prop)
    if not kitty_env.isrenderobj(obj) then
        return nil
    end
    return obj[prop]
end

kitty_env.setrenderproperty = function(obj, prop, value)
    if not kitty_env.isrenderobj(obj) then
        return
    end
    obj[prop] = value
end
-- end

-- execution listener
task.spawn(function()
    while task.wait() do
        local module_name = tostring(math.random(1, 1e9))
        local module = copyModule(module_name)

        local status = sendData("/ls?p=" .. client_pid .. "&c=" .. module_name, "")

        if status == 200 then
            local success, exec = pcall(require, module)
            if success and type(exec) == "function" then
                local env = setmetatable({
                    script = module
                }, { __index = kitty_env }) -- if key not found __index trying to find it in 'kitty_env' table
				env.script.Parent = nil
                pcall(setfenv, exec, env)
                task.spawn(exec)
            end
            module:Destroy()
        elseif status == 204 then
            module:Destroy()
        else
            task.wait()
        end
    end
end)
-- end

-- attaching notifier made by @byteblue credits him
pcall(function()
    local sound = Instance.new("Sound")
    sound.SoundId = "rbxassetid://17692186009"
    sound.Volume = 1
    sound.PlayOnRemove = true
    sound.Parent = sound_service
    sound:Destroy()
end)

pcall(function()
    local notificationBindable = Instance.new("BindableFunction")
    
    notificationBindable.OnInvoke = function(buttonText)
        if buttonText == "Dismiss" then end
    end

    starter_gui:SetCore("SendNotification", {
        Title = getexecutorname().." "..getexecutorversion(),
        Text = "Magically attached to the Roblox!",
        Icon = "rbxassetid://76384208066330",
        Duration = 10,
        Button1 = "Dismiss",
        Callback = notificationBindable
    })
end)