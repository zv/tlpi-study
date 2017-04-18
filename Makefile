# Makefile to build all practice exercises

DIRS = lib ch3 ch29 fileio

BUILD_DIRS = ${DIRS} ${CDIRS}

all:
	@ for dir in ${BUILD_DIRS}; do (cd $${dir}; ${MAKE}) ; done

allgen:
	@ for dir in ${BUILD_DIRS}; do (cd $${dir}; ${MAKE} allgen) ; done

clean:
	@ for dir in ${BUILD_DIRS}; do (cd $${dir}; ${MAKE} clean) ; done
