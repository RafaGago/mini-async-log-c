local projname             = "malc"
local repo                 = path.getabsolute ("../..")
local base_library         = repo .. "/dependencies/base_library"
local cmocka               = base_library .. "/dependencies/cmocka"
local base_library_include = base_library .. "/include"
local base_library_src     = base_library .. "/src"
local base_library_stage   = base_library .. "/build/stage/" .. os.get()
local repo_include         = repo .. "/include"
local repo_src             = repo .. "/src"
local repo_test_src        = repo .. "/test/src"
local repo_build           = repo .. "/build"
local stage                = repo_build .. "/stage/" .. os.get()
local build                = repo_build .. "/" .. os.get()
local version              = ".0.0.0"

local function os_is_posix()
  return os.get() ~= "windows"
end

solution "build"  
  configurations { "release", "debug" }
  location (build)
  
filter {"configurations:*"}  
  if os.get() == "linux" then
    defines {"BL_USE_CLOCK_MONOTONIC_RAW"}
  end

filter {"configurations:*release*"}  
  defines {"NDEBUG"}
  optimize "On"

filter {"configurations:*debug*"}
  flags {"Symbols"}
  defines {"DEBUG"}
  optimize "Off"
  
filter {"configurations:*"}
  flags {"MultiProcessorCompile"}
  includedirs { repo_include, repo_src, base_library_include }  

filter {"configurations:*"}
  if os_is_posix() then
    postbuildcommands {
      "cd %{cfg.buildtarget.directory} && "..
      "ln -sf %{cfg.buildtarget.name} " ..
      "%{cfg.buildtarget.name:gsub ('.%d+.%d+.%d+', '')}"    
    }
  end
--[[  
filter {"kind:SharedLib", "configurations:*debug*"}
  if os_is_posix() then
    targetextension (".so" .. ".d" .. version)
  end
  
filter {"kind:SharedLib", "configurations:*release*"}
  if os_is_posix() then
    targetextension (".so" .. version)
  end
]]--
filter {"kind:StaticLib", "configurations:*debug*"}
  if os_is_posix() then
    targetextension (".a" .. ".d" .. version )
  end
  targetdir (stage .. "/debug/lib")
  
filter {"kind:StaticLib", "configurations:*release*"}
  if os_is_posix() then
    targetextension (".a" .. version)
  end
  targetdir (stage .. "/release/lib")
  
filter {"kind:ConsoleApp", "configurations:*debug*"}
  if os_is_posix() then
    targetextension ("_d" .. version)
    --libdirs broken on alpha6
  end
  links { 
    base_library_stage .. "/debug/lib/base_library.d",
  } 
  targetdir (stage .. "/debug/bin")
  
filter {"kind:ConsoleApp", "configurations:*release*"}
  if os_is_posix() then
    targetextension (version)
    --libdirs broken on alpha6
  end
  links { 
    base_library_stage .. "/release/lib/base_library"
  } 
  targetdir (stage .. "/release/bin")

--toolset filters and file filters broken in alpha
--dirty workaround (specifying compiler for each os) follows ...

-- WORKAROUND --
local function is_gcc()
  return os.get() ~= "windows"
end

filter {"language:C"}  
  if is_gcc() then
    buildoptions {"-Wfatal-errors"}
    buildoptions {"-std=c11"}
    defines {"_XOPEN_SOURCE=700"}
  end
  
filter {"configurations:*", "kind:*Lib"}
  if is_gcc() then
    buildoptions {"-fvisibility=hidden"}
  end

filter {"kind:ConsoleApp"}
  if is_gcc() then
    links {"pthread", "rt"}
  end
-- WORKAROUND END --

project (projname)
  kind "ConsoleApp"
  language "C"
  files {repo_src .. "/**.c", repo_src .. "/**.h"}
--[[  
project (projname .. "_test")
  kind "ConsoleApp"
  language "C"
  includedirs {cmocka .. "/include", repo_test_src }
  files {repo_test_src .. "/task_queue/**"}
  links {cmocka .. "/lib/cmocka"} 
    
project (projname .. "_stress_test")
  kind "ConsoleApp"
  language "C"
  includedirs {cmocka .. "/include", repo_test_src }
  files {repo_test_src .. "/stress/**"}
]]--
