include('shared.lua')
local GunmanShared = include("gunman_shared.lua")

SWEP.CrosshairMaterial = "gunman/sprites/beamgun_crosshair"

SWEP.MenuLabels = {
	Range = "RANGE",
	Power = "POWER",
	Accuracy = "ACCURACY",
	Lightning = "LIGHTNING",
}

SWEP.Range = {
	"TOUCH TAZER",
	"SHORT TAZER",
	"MEDIUM BEAM",
	"LONG BEAM",
}

SWEP.PowerAndAccuracy = {
	"LOW",
	"MEDIUM",
	"HIGH",
	"HIGHEST",
}

SWEP.Lightning = {
	"BEAM",
	"CHAIN",
	"BALL",
}

SWEP.WepSelectIcon = surface.GetTextureID("gunman/vgui/hud_beamgun")  

function SWEP:TextLabelWarning(string, color)
	local targetRes = math.min(ScrW(), 1920) / 1920

	local textLabel_W = (ScrW() - targetRes) / 2
	local textLabel_H = ScrH() - (100 * targetRes ^ 0.8) 

	local font = "Gunman_HUDFont" 

	if ScrH() < 900 then
		font = "Gunman_HUDFont_small" 
	end

	draw.SimpleText(string, font, textLabel_W, textLabel_H, color, TEXT_ALIGN_CENTER, TEXT_ALIGN_TOP)
end

function SWEP:CustomDrawHUD()	
	self.weaponTemp = self:GetNW2Float("weaponTemp", 0)
	self.Malfunction = self:GetNW2Bool("WeaponMalfunctioned", false)

	self.FakePower = 1 + #self.PowerAndAccuracy - self.CurrentPowerAndAccuracy
	self.FakeAccuracy = math.abs(#self.PowerAndAccuracy 
		- (self.CurrentPowerAndAccuracy + #self.PowerAndAccuracy))

	local menuLabels = {
		{ label = self.MenuLabels.Range,     value = self.Range[self.CurrentRange],            offset = 0,   optionIndex = 1 },
		{ label = self.MenuLabels.Power,     value = self.PowerAndAccuracy[self.FakePower],    offset = 60,  optionIndex = 2 },
		{ label = self.MenuLabels.Accuracy,  value = self.PowerAndAccuracy[self.FakeAccuracy], offset = 100, optionIndex = 2 },
		{ label = self.MenuLabels.Lightning, value = self.Lightning[self.CurrentLightning],    offset = 160, optionIndex = 3 },
	}

	self:DrawMenuHUD(menuLabels)

	local targetRes = math.min(ScrW(), 1920) / 1920

	local baseW, baseH = ScrW(), ScrH()
	local texSizeX = 48 * 4 * targetRes
	local texSizeY = 48 * targetRes

	local x = (baseW - texSizeX) / 2
	local y = baseH - texSizeY * 1.5
	
	local scaleRectX = 10 + texSizeX * (self.weaponTemp / 160)

	local warnColor = Color(0, 255, 0, 255)
	local warningText = ""
	
	if self.weaponTemp > 120 then
		warningText = self.Malfunction and "MALFUNCTION" or "DANGER"
		warnColor = Color(255, 0, 0, 255)

		halo.Add({self.Owner:GetViewModel()}, Color(80, 60, 255), 8, 8, 2, true, true)
	elseif self.weaponTemp > 80 then
		warningText = "WARNING"
		warnColor = Color(255, 255, 0, 255)
	end

	if warningText ~= "" then
		self:TextLabelWarning(warningText, warnColor)
	end
	
	surface.SetDrawColor(warnColor)
	surface.DrawRect(x, y, scaleRectX, texSizeY)
	
	surface.SetMaterial(Material("gunman/vgui/640_temprature"))
	surface.DrawTexturedRect(x, y, texSizeX, texSizeY)
end

function SWEP:DrawBeams(startPos, endPos, beamConfigs)
	for _, config in ipairs(beamConfigs) do
		GunmanShared.DoSegmentedBeam(
			startPos,
			endPos,
			config.width,
			config.segments,
			config.noise,
			config.endNoise,
			config.material,
			config.color
		)
	end
end

SWEP.BeamConfigs = {
    [3] = {  
        [4] = { 
            { width = 8, segments = 2, noise = 2, endNoise = 2, material = "gunman/sprites/xbeam1", color = Color(255, 80, 255, 255) },
            { width = 2, segments = 16, noise = 6, endNoise = 2, material = "gunman/sprites/xbeam1", color = Color(255, 255, 100, 255) },
        },
        [3] = { 
            { width = 10, segments = 2, noise = 2, endNoise = 10, material = "gunman/sprites/xbeam1", color = Color(255, 200, 255, 255) },
            { width = 2, segments = 16, noise = 8, endNoise = 10, material = "gunman/sprites/xbeam1", color = Color(255, 255, 100, 255) },
        },
        [2] = { 
            { width = 10, segments = 2, noise = 2, endNoise = 30, material = "gunman/sprites/xbeam1", color = Color(255, 200, 255, 255) },
            { width = 2, segments = 16, noise = 8, endNoise = 30, material = "gunman/sprites/xbeam1", color = Color(255, 255, 100, 255) },
        },
        [1] = { 
            { width = 10, segments = 2, noise = 2, endNoise = 50, material = "gunman/sprites/xbeam1", color = Color(255, 200, 255, 255) },
            { width = 2, segments = 16, noise = 8, endNoise = 50, material = "gunman/sprites/xbeam1", color = Color(255, 255, 100, 255) },
        },
    },
    [2] = {  
        [4] = { 
            { width = 8, segments = 12, noise = 16, endNoise = 40, material = "gunman/sprites/xbeam1", color = Color(200, 100, 255, 255) },
            { width = 4, segments = 12, noise = 2, endNoise = 2, material = "gunman/sprites/xbeam1", color = Color(255, 255, 100, 255) },
            { width = 2, segments = 12, noise = 8, endNoise = 20, material = "gunman/sprites/xbeam1", color = Color(255, 255, 100, 255) },
        },
        [3] = { 
            { width = 8, segments = 12, noise = 16, endNoise = 40, material = "gunman/sprites/xbeam1", color = Color(255, 200, 200, 255) },
            { width = 2, segments = 12, noise = 8, endNoise = 40, material = "gunman/sprites/xbeam1", color = Color(255, 255, 100, 255) },
        },
        [2] = { 
            { width = 10, segments = 12, noise = 16, endNoise = 60, material = "gunman/sprites/xbeam1", color = Color(255, 255, 150, 255) },
            { width = 4, segments = 12, noise = 8, endNoise = 60, material = "gunman/sprites/xbeam1", color = Color(255, 255, 150, 255) },
        },
        [1] = { 
            { width = 14, segments = 12, noise = 16, endNoise = 60, material = "gunman/sprites/xbeam1", color = Color(255, 255, 255, 255) },
            { width = 6, segments = 12, noise = 8, endNoise = 60, material = "gunman/sprites/xbeam1", color = Color(255, 255, 255, 255) },
        },
    },
    [1] = {  
        [4] = { 
            { width = 5, segments = 16, noise = 16, endNoise = 120, material = "gunman/sprites/xbeam1", color = Color(255, 100, 100, 255) },
            { width = 2, segments = 16, noise = 8, endNoise = 120, material = "gunman/sprites/xbeam1", color = Color(255, 255, 100, 255) },
        },
        [3] = { 
            { width = 8, segments = 16, noise = 16, endNoise = 120, material = "gunman/sprites/xbeam1", color = Color(255, 255, 100, 255) },
            { width = 2, segments = 16, noise = 8, endNoise = 120, material = "gunman/sprites/xbeam1", color = Color(255, 255, 100, 255) },
        },
        [2] = { 
            { width = 10, segments = 16, noise = 16, endNoise = 120, material = "gunman/sprites/xbeam1", color = Color(255, 255, 150, 255) },
            { width = 4, segments = 16, noise = 8, endNoise = 120, material = "gunman/sprites/xbeam1", color = Color(255, 255, 150, 255) },
        },
        [1] = { 
            { width = 14, segments = 16, noise = 16, endNoise = 120, material = "gunman/sprites/xbeam1", color = Color(255, 255, 255, 255) },
            { width = 6, segments = 16, noise = 8, endNoise = 120, material = "gunman/sprites/xbeam1", color = Color(255, 255, 255, 255) },
        },
    },
}

function SWEP:RenderLightningBeam(startPos, endPos)
	if self.CurrentRange <= 2 and self.tazerTazing then
		self:DrawBeams(startPos, endPos, {
			{width = 4, segments = 16, noise = 8, endNoise = 256, material = "gunman/sprites/xbeam1", color = Color(255, 255, 150, 255)},
			{width = 4, segments = 16, noise = 16, endNoise = 256, material = "gunman/sprites/xbeam1", color = Color(255, 255, 150, 255)},
			{width = 4, segments = 16, noise = 8, endNoise = 128, material = "gunman/sprites/xbeam1", color = Color(255, 255, 150, 255)},
			{width = 4, segments = 16, noise = 16, endNoise = 128, material = "gunman/sprites/xbeam1", color = Color(255, 255, 150, 255)},
			{width = 4, segments = 16, noise = 8, endNoise = 129, material = "gunman/sprites/xbeam1", color = Color(255, 255, 150, 255)},
			{width = 4, segments = 16, noise = 16, endNoise = 128, material = "gunman/sprites/xbeam1", color = Color(255, 255, 150, 255)},
			
		})
	end

	local currentRangeMax = 3

	if self.CurrentLightning ~= 3 and not self.tazerTazing then
		local rangeConfig = self.BeamConfigs[math.min(self.CurrentRange, currentRangeMax)]
		if rangeConfig then
			local powerConfig = rangeConfig[self.CurrentPowerAndAccuracy]
			if powerConfig then
				self:DrawBeams(startPos, endPos, powerConfig)
			end
		end
	end

	if self.CurrentLightning == 3 then
		self.BallWasLaunched = self:GetNW2Bool("BallWasLaunched", false)

		if not self.BallWasLaunched then
			self:DrawBeams(startPos, endPos, {
				{width = 6, segments = 8, noise = 1, endNoise = 256, material = "gunman/sprites/xbeam1", color = Color(255, 50, 255, 255)},
				{width = 4, segments = 16, noise = 32, endNoise = 320, material = "gunman/sprites/xbeam1", color = Color(255, 255, 150, 255)},
				{width = 16, segments = 2, noise = 1, endNoise = 0, material = "gunman/sprites/xbeam1", color = Color(50, 150, 255, 255)},
			})
		end
	end
end

function SWEP:GetBeamEndPos()
	if self.CurrentLightning == 3 then
		return GunmanShared.ClampedTracerHitPos(self, 128)	
	end

	return GunmanShared.ClampedTracerHitPos(self, 128 * (self.CurrentRange ^ 2))
end

function SWEP:DrawWorldModel()
	self.Weapon:DrawModel()

	self.HasFired = self:GetNW2Bool("HasFired", false)
	self.CurrentRange = self:GetNW2Int("CurrentRange", 3)
	self.CurrentPowerAndAccuracy = self:GetNW2Int("CurrentPowerAndAccuracy", 2)
	self.CurrentLightning = self:GetNW2Int("CurrentLightning", 1)
	self.tazerTazing = self:GetNW2Bool("tazerTazing", false)

	if self.Owner:IsValid() == false then return end
	if self.HasFired == false then return end

	startPos = self.Owner:GetActiveWeapon():GetAttachment(
			self.Owner:GetActiveWeapon():LookupAttachment("muzzle")).Pos 

	local endPos = self:GetBeamEndPos()

	self:RenderLightningBeam(startPos, endPos)
end

function SWEP:CustomViewModelDrawn()
	self.HasFired = self:GetNW2Bool("HasFired", false)
	self.CurrentRange = self:GetNW2Int("CurrentRange", 3)
	self.CurrentPowerAndAccuracy = self:GetNW2Int("CurrentPowerAndAccuracy", 2)
	self.CurrentLightning = self:GetNW2Int("CurrentLightning", 1)
	self.tazerTazing = self:GetNW2Bool("tazerTazing", false)

	if self.Owner:IsValid() == false or self.HasFired == false then return end
	
	local startPos = self:GetViewModelBonePos(30)
	
	local endPos = self:GetBeamEndPos()

	self:RenderLightningBeam(startPos, endPos)
end