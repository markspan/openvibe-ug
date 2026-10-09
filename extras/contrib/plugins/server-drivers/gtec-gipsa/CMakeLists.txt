# g.tec drivers contributed by Gipsa-lab (VIBS team).
#
# common/ builds nothing of its own -- it only carries ringbuffer.h, which both drivers below
# include and the other g.tec drivers can share.
add_subdirectory(gusbamp)
add_subdirectory(unicorn)
