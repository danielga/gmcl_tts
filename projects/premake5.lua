if not os.istarget("windows") then
	error("Your operating system is not supported")
end

newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
	value = "path to garrysmod_common directory"
})

local gmcommon = _OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON")
assert(gmcommon ~= nil, "you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory")

include(gmcommon)

CreateWorkspace({name = "tts"})
	CreateProject({serverside = false})
		IncludeLuaShared()
		defines({"WINVER=0x0600", "_WIN32_WINNT=0x0600"})
		links("sapi")
