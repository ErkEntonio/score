set(CMAKE_BUILD_TYPE Release CACHE INTERNAL "")
set(SCORE_PCH True)
set(SCORE_DEPLOYMENT_BUILD True)
set(SCORE_STATIC_PLUGINS True)
set(Boost_USE_STATIC_LIBS ON)
set(SCORE_ENABLE_LTO False)
set(SCORE_AUDIO_PLUGINS True CACHE INTERNAL "")

include(default-plugins)

