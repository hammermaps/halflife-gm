local gunmanData = include("gunman_data.lua") 
local utilFuncs = include("gunman_util.lua")

utilFuncs.registerDecals(gunmanData.decals)
utilFuncs.registerKillicons(gunmanData.killicons)

utilFuncs.RegisterSounds(gunmanData.WeaponChannel, gunmanData.WeaponSoundSettings)
utilFuncs.RegisterSounds(gunmanData.ItemChannel, gunmanData.ItemSoundSettings)
utilFuncs.RegisterSounds(gunmanData.AutoChannel, gunmanData.AutoSoundSettings)

utilFuncs.RegisterAmmoTypes(gunmanData.ammoTypes, gunmanData.defaultType)


    

