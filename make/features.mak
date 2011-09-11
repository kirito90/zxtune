ifdef support_waveout
$(platform)_definitions += WIN32_WAVEOUT_SUPPORT
$(platform)_libraries += winmm
endif

ifdef support_oss
$(platform)_definitions += OSS_SUPPORT
endif

ifdef support_alsa
$(platform)_definitions += ALSA_SUPPORT
endif

ifdef support_aylpt_dlportio
$(platform)_definitions += DLPORTIO_AYLPT_SUPPORT
endif

ifdef support_sdl
$(platform)_definitions += SDL_SUPPORT
mingw_libraries += winmm gdi32 dxguid
endif

ifdef support_zlib
$(platform)_definitions += ZLIB_SUPPORT
$(platform)_libraries += z
windows_include_dirs += $(path_step)/3rdparty/zlib
windows_depends += 3rdparty/zlib
endif
