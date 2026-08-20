# g.tec drivers contributed by Gipsa-lab (VIBS team).
#
# common/ builds nothing of its own -- it only carries ringbuffer.h, which gusbamp/ includes and the
# other g.tec drivers can share. unicorn/ is still on the retired ovasIDriver interface and awaits
# the same port gusbamp/ has had.
add_subdirectory(gusbamp)
