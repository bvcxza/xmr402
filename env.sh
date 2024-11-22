
conda activate $(basename $PWD)

# functions

bld_xmr()
{
	[[ -d build_xmr ]] || mkdir build_xmr
	[[ -d $INSTALL_DIR ]] || mkdir $INSTALL_DIR
	(cd external/libexpat/expat && \
		./buildconf.sh && ./configure --prefix=$INSTALL_DIR --enable-static --disable-shared --with-pic=yes && \
		make -j$(($(nproc) / 2)) install) && \
	(cd external/unbound && \
		./configure --prefix=$INSTALL_DIR --with-libexpat=$INSTALL_DIR --with-libunbound-only \
			--enable-static-exe --enable-static --disable-shared --with-pic=yes && \
		make -j$(($(nproc) / 2)) install) && \
	(cd build_xmr && \
		cmake --fresh -G Ninja -D BUILD_TESTS=OFF -D STATIC=OFF -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=$INSTALL_DIR -D USE_DEVICE_TREZOR=OFF ../external/monero && \
		ninja -l1.5 wallet && cp lib/*.a $INSTALL_DIR/lib)
}

cnf()
{
	[[ -d build ]] || mkdir build
	(cd build && cmake --fresh -G Ninja $@ ..)
}

bld()
{
	[[ -d build ]] || cnf
	(cd build && ninja $@)
}

run()
{
	[[ -d build ]] || bld
	./build/xmr402 $@
}


export INSTALL_DIR="$(pwd)/install_xmr"
export CMAKE_PREFIX_PATH=$INSTALL_DIR${CMAKE_PREFIX_PATH+:$CMAKE_PREFIX_PATH}
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
