dynamic_name := benchmark
dirs.root := ../../../../../../..
source_dirs := .

libraries = benchmark 
libraries.common = devices_aym devices_z80 l10n_stub sound tools
libraries.3rdparty = z80ex

depends := apps/benchmark/core

libraries.android += log

include $(dirs.root)/makefile.mak

install_$(platform):
	$(call copyfile_cmd,$(target),$(DESTDIR))

install_debug:
	$(call copyfile_cmd,$(target).pdb,$(DESTDIR))
