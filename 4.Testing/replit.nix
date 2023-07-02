{ pkgs }: {
	deps = [
    pkgs.gcc-unwrapped
		pkgs.valgrind
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
	];
}