
完全虚拟化和半虚拟化。
=====================
在完全虚拟化 中，来宾操作系统运行在位于物理机器上的 hypervisor 之上。来宾操作系统并不知道它已被虚拟化，并且不需要任何更改就可以在该配置下工作。
在半虚拟化 中，来宾操作系统不仅知道它运行在 hypervisor 之上，还包含让来宾操作系统更高效地过渡到 hypervisor 的代码。
在完全虚拟化模式中，hypervisor 必须模拟设备硬件，它是在会话的最低级别进行模拟的（例如，网络驱动程序）。尽管在该抽象中模拟很干净，但它同时也是最低效、最复杂的。
在半虚拟化模式中，来宾操作系统和 hypervisor 能够共同合作，让模拟更加高效。半虚拟化方法的缺点是操作系统知道它被虚拟化，并且需要修改才能工作。

virtio 是对半虚拟化 hypervisor 中的一组通用模拟设备的抽象。
来宾操作系统能够实现一组通用的接口，在一组后端驱动程序之后采用特定的设备模拟。后端驱动程序不需要是通用的，因为它们只实现前端所需的行为。

除了前端驱动程序（在来宾操作系统中实现）和后端驱动程序（在 hypervisor 中实现）之外，virtio 还定义了两个层来支持来宾操作系统到 hypervisor 的通信。在顶级（称为 virtio）的是虚拟队列接口，它在概念上将前端驱动程序附加到后端驱动程序。驱动程序可以使用 0 个或多个队列，具体数量取决于需求。


drivers/net/virtio_net.c
drivers/scsi/virtio_scsi.c
drivers/block/virtio_blk.c
drivers/s390/kvm/virtio_ccw.c
drivers/virtio/virtio_balloon.c
drivers/virtio/virtio_pci.c
drivers/virtio/virtio_ring.c
drivers/virtio/virtio_mmio.c
drivers/rpmsg/virtio_rpmsg_bus.c
drivers/char/virtio_console.c


virtio-blk   virtio-net virtio-pci
    \            |          /
     \           |         /
             virtio -- drivers/virtio/virtio.c
                 |
            transport -- drivers/virtio/virtio_ring.c
                 |
           virtio backend drivers
 
          
Virtqueue
=====================
每个设备拥有多个 virtqueue 用于大块数据的传输。virtqueue 是一个简单的队列，guest 把 buffers 插入其中，每个 buffer 都是一个分散-聚集数组。
virtqueue 的数目根据设备的不同而不同，比如 block 设备有一个 virtqueue,network 设备有 2 个 virtqueue,一个用于发送数据包，一个用于接收数据包。Balloon 设备有 3 个 virtqueue.


Vring
=====================
virtio_ring 是 virtio 传输机制的实现，vring 引入 ring buffers 来作为我们数据传输的载体。


全虚拟化 QUEM模拟I/O设备的基本原理和优缺点:
=====================

使用QEMU模拟I/O的情况下，当客户机中的设备驱动程序（device driver）发起I/O操作请求之时，KVM模块中的I/O操作捕获代码会拦截这次I/O请求，然后经过处理后将本次I/O请求的信息存放到I/O共享页，并通知用户控件的QEMU程序。QEMU模拟程序获得I/O操作的具体信息之后，交由硬件模拟代码来模拟出本次的I/O操作，完成之后，将结果放回到I/O共享页，并通知KVM模块中的I/O操作捕获代码。最后，由KVM模块中的捕获代码读取I/O共享页中的操作结果，并把结果返回到客户机中。当然，这个操作过程中客户机作为一个QEMU进程在等待I/O时也可能被阻塞。另外，当客户机通过DMA（Direct Memory Access）访问大块I/O之时，QEMU模拟程序将不会把操作结果放到I/O共享页中，而是通过内存映射的方式将结果直接写到客户机的内存中去，然后通过KVM模块告诉客户机DMA操作已经完成。

QEMU模拟I/O设备的方式，其优点是可以通过软件模拟出各种各样的硬件设备，包括一些不常用的或者很老很经典的设备（如4.5节中提到RTL8139的网卡），而且它不用修改客户机操作系统，就可以实现模拟设备在客户机中正常工作。在KVM客户机中使用这种方式，对于解决手上没有足够设备的软件开发及调试有非常大的好处。而它的缺点是，每次I/O操作的路径比较长，有较多的VMEntry、VMExit发生，需要多次上下文切换（context switch），也需要多次数据复制，所以它的性能较差。


半虚拟化 virtio的基本原理和优缺点:
=====================
其中前端驱动(frondend，如virtio-blk、virtio-net等)是在客户机中存在的驱动程序模块，而后端处理程序（backend）是在QEMU中实现的。在这前后端驱动之间，还定义了两层来支持客户机与QEMU之间的通信。其中，“virtio”这一层是虚拟队列接口，它在概念上将前端驱动程序附加到后端处理程序。一个前端驱动程序可以使用0个或多个队列，具体数量取决于需求。例如，virtio-net网络驱动程序使用两个虚拟队列（一个用于接收，另一个用于发送），而virtio-blk块驱动程序仅使用一个虚拟队列。虚拟队列实际上被实现为跨越客户机操作系统和hypervisor的衔接点，但它可以通过任意方式实现，前提是客户机操作系统和virtio后端程序都遵循一定的标准，以相互匹配的方式实现它。而virtio-ring实现了环形缓冲区(ring buffer),用于保存前端驱动和后端处理程序执行的信息，并且它可以一次性保存前端驱动的多次I/O请求，并且交由后端去动去批量处理，最后实际调用宿主机中设备驱动实现物理上的I/O操作，这样做就可以根据约定实现批量处理而不是客户机中每次I/O请求都需要处理一次，从而提高客户机与hypervisor信息交换的效率。

Virtio半虚拟化驱动的方式，可以获得很好的I/O性能，其性能几乎可以达到和native(即:非虚拟化环境中的原生系统)差不多的I/O性能。所以，在使用KVM之时，如果宿主机内核和客户机都支持virtio的情况下，一般推荐使用virtio达到更好的性能。当然，virtio的也是有缺点的，它必须要客户机安装特定的Virtio驱动使其知道是运行在虚拟化环境中，且按照Virtio的规定格式进行数据传输，不过客户机中可能有一些老的Linux系统不支持virtio和主流的Windows系统需要安装特定的驱动才支持Virtio。不过，较新的一些Linux发行版(如RHEL 6.3、Fedora 17等)默认都将virtio相关驱动编译为模块，可直接作为客户机使用virtio，而且对于主流Windows系统都有对应的virtio驱动程序可供下载使用。


virtio是对半虚拟化hypervisor中的一组通用模拟设备的抽象.该设置还允许hypervisor导出一组通用的模拟设备,并通过一个通用的应用程序接口(API)让它们变得可用.有了半虚拟化hypervisor之后,来宾操作系统能够实现一组通用的接口,在一组后端驱动程序之后采用特定的设备模拟.后端驱动程序不需要是通用的,因为它们只实现前端所需的行为.


