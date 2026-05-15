local GunmanShared = include("gunman_shared.lua")
include('shared.lua')

SWEP.CrosshairMaterial = "gunman/sprites/mechagun_crosshair"

SWEP.WepSelectIcon = surface.GetTextureID("gunman/vgui/hud_mechagun")  

function SWEP:WarningText(string, color)
	local baseW, baseH = ScrW(), ScrH() / 1
	local scaleH = baseH / 1440

	local x = baseW / 2
	local y = scaleH + (baseH / 1.11) 

	local textX, textY = x, y 

	draw.SimpleText(string, "Gunman_HUDFont", textX, textY, color, TEXT_ALIGN_CENTER, TEXT_ALIGN_TOP)
end

function SWEP:DrawHUD()	
	local baseW, baseH = ScrW(), ScrH() / 1
	local scaleH = baseH / 1440
	local texSizeX, texSizeY = 48 * 4, 48

	local x = (baseW - texSizeX) / 2
	local y = baseH - texSizeY * 1.5

	if CurTime() > self.NextThinkClientTime then
		self.Malfunction = self:GetNW2Bool("WeaponMalfunctioned", false)
		self.weaponTemp = self:GetNW2Int("WeaponTemp", 0)
	
		self.NextThinkClientTime = CurTime() + 0.1
	end

	local statusText, statusColor, showHalo
	local barColor = Color(0, 255, 0, 255)

	if self.Malfunction == true then
		statusText, statusColor, barColor, showHalo = "MALFUNCTION", Color(255, 0, 0, 255), Color(255, 0, 0, 255), true
	elseif self.weaponTemp > 120 then
		statusText, statusColor, barColor, showHalo = "DANGER", Color(255, 0, 0, 255), Color(255, 0, 0, 255), true
	elseif self.weaponTemp > 80 then
		statusText, statusColor, barColor = "WARNING", Color(255, 255, 0, 255), Color(255, 255, 0, 255)
	end

	if statusText then
		self:WarningText(statusText, statusColor)
	end

	if showHalo then
		halo.Add({self.Owner:GetViewModel()}, Color(255, 110, 0) , 8, 8, 2, true, true)
	end

	surface.SetDrawColor(barColor.r, barColor.g, barColor.b, 255)

	surface.DrawRect(x + 16, y + 8, self.weaponTemp, 32)

	surface.SetDrawColor(255, 255, 255, 255) 
	surface.SetMaterial(Material("gunman/vgui/640_temprature")) 
	surface.DrawTexturedRect(x, y, texSizeX, texSizeY)   
end

function SWEP:CustomViewModelDrawn()
    net.Receive("TriggerMuzzleFlash" .. tostring(self), function()
		self.muzzleFlashLifeTime = net.ReadFloat()
	end)
	
	if CurTime() < self.muzzleFlashLifeTime then
		local viewPos = self:GetViewModelBonePos(32)

		render.SetMaterial(Material("gunman/sprites/muzzleflash2"))
		render.DrawSprite(viewPos, 32, 32, Color(255, 255, 255, 255))
	end

	self.FireModeTurbo = self:GetNW2Bool("FireModeTurbo", false)

	if CurTime() < self.muzzleFlashLifeTime and self.FireModeTurbo then
		local viewPos = self:GetViewModelBonePos(36)

		render.SetMaterial(Material("gunman/sprites/muzzleflash2"))
		render.DrawSprite(viewPos, 32, 32, Color(255, 255, 255, 255))
	end
end

function SWEP:DrawWorldModel()
	self.Weapon:DrawModel()

    net.Receive("TriggerMuzzleFlash" .. tostring(self), function()
		self.muzzleFlashLifeTime = net.ReadFloat()
	end)

	if CurTime() < self.muzzleFlashLifeTime then
		local weapon = self.Owner:GetActiveWeapon()
		local attachment = weapon:GetAttachment(weapon:LookupAttachment("muzzle"))

		if attachment then
			render.SetMaterial(Material("gunman/sprites/muzzleflash2"))
			render.DrawSprite(attachment.Pos, 12, 12, Color(255, 255, 255, 255))
		end
	end
end
