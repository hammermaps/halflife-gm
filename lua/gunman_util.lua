AddCSLuaFile()

local F = {}

function F.registerDecals(decals)
	for i = 1, #decals do
		game.AddDecal(decals[i][1], decals[i][2])
	end
end

function F.RegisterSounds(soundsTable, settingsTable)
	for _, snd in ipairs(soundsTable) do
		local final = table.Copy(settingsTable)

		for k, v in pairs(snd) do
			final[k] = v
		end

		if type(final.sound) == "table" then
			for _, path in ipairs(final.sound) do
				util.PrecacheSound(path)
			end
		elseif type(final.sound) == "string" then
			util.PrecacheSound(final.sound)
		end

		sound.Add(final)
	end
end

function F.RegisterAmmoTypes(ammoTypes, defaultType)
	for _, ammo in ipairs(ammoTypes) do
		local final = table.Copy(defaultType)

		for k, v in pairs(ammo) do
			final[k] = v
		end

		game.AddAmmoType(final)
	end
end

function F.registerKillicons(killicons)
	if not CLIENT then return end
	for i = 1, #killicons do
		killicon.Add(killicons[i][1], killicons[i][2], color_white)
	end
end


return F