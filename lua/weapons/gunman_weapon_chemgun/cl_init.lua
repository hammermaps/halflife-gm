include('shared.lua')

SWEP.CrosshairMaterial = "gunman/sprites/chemgun_crosshair"

SWEP.WepSelectIcon = surface.GetTextureID("gunman/vgui/hud_chemgun")  
	
SWEP.ChemMixtureLabels = {
	Acid = "ACID",
	Neutral = "NEUTRAL",
	Base = "BASE",
	Pressure = "PRESSURE",
}

function SWEP:TextLabel(string, var, offset, selected)
	local targetRes = ScrW() / 1920

	local resWidthScale = 6 / (targetRes ^ 1.333)
	local resHeightScale = 2.2 / (targetRes ^ 0.17)

	local textLabelPos_W = 440
	local textVarOffset = 384

	local textLabel_W = (ScrW() - textLabelPos_W) / resHeightScale
	local textLabel_H = (ScrH() / resWidthScale) + offset

	local textVar_W = textLabel_W + textVarOffset

	local font = selected and "Gunman_HUDFont_Selected" or "Gunman_HUDFont_Unselected"
	local color = selected and Color(0, 255, 0) or Color(210, 255, 210)
	
	draw.SimpleText(string, font, textLabel_W, textLabel_H, color, TEXT_ALIGN_LEFT, TEXT_ALIGN_TOP)
end

function SWEP:CustomDrawHUD()
	local menuLabels = {
		{ label = self.ChemMixtureLabels.Acid, value = self.CurrentAcid, offset = 0 },
		{ label = self.ChemMixtureLabels.Neutral, value = self.CurrentNeutral, offset = 60 },
		{ label = self.ChemMixtureLabels.Base, value = self.CurrentBase, offset = 120 },
		{ label = self.ChemMixtureLabels.Pressure, value = self.CurrentPressure, offset = 180 },
	}

	self:DrawMenuHUD(menuLabels)
	
	surface.SetDrawColor(255, 255, 255) 

	if self:IsMenuOpen() then
		local targetRes = ScrW() / 1920
		local resWidthScale = 6 / (targetRes ^ 1.333)
		local rectH = (ScrH() / resWidthScale) 

		local x = ScrW() / 1.85
		local y = rectH + 358

		local vialMidMats = {     
			"gunman/sprites/png/vial_middle.png",        
			"gunman/sprites/png/vial_middle_full.png",    
		}
		local vialLeftMats = {
			"gunman/sprites/png/vial_left.png",         
			"gunman/sprites/png/vial_left_full.png",         
		}
		local vialRightMats = { 
			"gunman/sprites/png/vial_right.png",   
			"gunman/sprites/png/vial_right_full.png",
		}

		local chemStates = {
			self.CurrentAcid,
			self.CurrentNeutral,
			self.CurrentBase
		}

		local vialMats = {
			[1] = vialLeftMats,
			[2] = vialMidMats,
			[3] = vialMidMats,
			[4] = vialRightMats
		}
		
		local drawColors = {
			[1] = {0, 255, 0},   -- Acid = Green
			[2] = {0, 0, 255},   -- Neutral = Blue
			[3] = {255, 0, 0}    -- Base = Red
		}
		
		for col = 1, 4 do 
			for row = 1, 3 do 
				local index = (chemStates[row] >= col) and 2 or 1
				local yOffset = -422 + (60 * row)
				local xOffset = x + (48 * (col - 1))
		
				if index == 2 then
					local r, g, b = unpack(drawColors[row])
					surface.SetDrawColor(r, g, b)
				else
					surface.SetDrawColor(255, 145, 0)
				end
		
				surface.SetMaterial(Material(vialMats[col][index]))
				surface.DrawTexturedRect(xOffset, y + yOffset, 48, 48)
			end
		end

		local pressureScaleMats = {
			"gunman/sprites/png/scale.png",         
			"gunman/sprites/png/scale_pointer.png"    
		}

		local positions = {
			{ x = x + 5 },
			{ x = x + 53 },
			{ x = x + (50.5 * 2) },
			{ x = x + (49.75 * 3) },
			{ x = x + (49.25 * 4) }
		}

		for i = 1, 5 do
			local index = (self.CurrentPressure == i) and 2 or 1 

			if index == 2 then 
				surface.SetDrawColor(251, 255, 0) 
			else
				surface.SetDrawColor(255, 145, 0) 
			end

			surface.SetMaterial(Material(pressureScaleMats[index]))
			surface.DrawTexturedRect(positions[i].x, y + -175, 48, 48)
		end
	end
end
