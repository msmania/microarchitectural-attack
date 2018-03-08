all:
	@pushd 01_meltdown_toy & nmake /nologo & popd
	@pushd 02_spectre_toy & nmake /nologo & popd
	@pushd 03_meltdown_full & nmake /nologo & popd
	@pushd 04_spectre_full & nmake /nologo & popd

clean:
	@pushd 01_meltdown_toy & nmake /nologo clean & popd
	@pushd 02_spectre_toy & nmake /nologo clean & popd
	@pushd 03_meltdown_full & nmake /nologo clean & popd
	@pushd 04_spectre_full & nmake /nologo clean & popd
