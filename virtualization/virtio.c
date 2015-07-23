

virtio-blk为半虚拟驱动，virtio-blk请求处理过程如下：
1.客户机（virtio-blk设备驱动）读写数据方式vring队列
2.客户机执行Virtqueue队列函数kick通知host宿主机（通过virtio-pci硬件寄存器发送通知）
3.宿主机host截获通知信息
4.宿主机host从vring队列获取读写请求（vring队列内容涉及地址为客户机物理地址）
5.宿主机host处理读写请求
6.宿主机host处理结果添加到vring队列
7.宿主机host发送中断（通过virtio-pci中断）

drivers/virtio/virtio_pci.c


客户机与宿主机通知

notify函数用于通知host主机队列里面已经有消息存在了，s390采用是hypercall ，而其他体系结构使用写寄存器来通知host主机。

module_pci_driver(virtio_pci_driver);
    .probe      = virtio_pci_probe,
        vp_dev->vdev.config = &virtio_pci_config_ops;
            .find_vqs   = vp_find_vqs       
                vp_try_to_find_vqs
                    setup_vq
                        vq = vring_new_virtqueue(index, info->num, VIRTIO_PCI_VRING_ALIGN, vdev, true, info->queue, vp_notify, callback, name);
                                
vp_notify
    iowrite16(vq->index, vp_dev->ioaddr + VIRTIO_PCI_QUEUE_NOTIFY);

virtio-blk注册要截获的端口
pci_request_regions
    pci_request_selected_regions
        __pci_request_selected_regions
            __pci_request_region
                request_region

static const struct virtio_config_ops virtio_mmio_config_ops = {
    .get        = vm_get,
    .set        = vm_set,
    .get_status = vm_get_status,
    .set_status = vm_set_status,
    .reset      = vm_reset,
    .find_vqs   = vm_find_vqs,
    .del_vqs    = vm_del_vqs,
    .get_features   = vm_get_features,
    .finalize_features = vm_finalize_features,
    .bus_name   = vm_bus_name,
};


宿主机发送中断通知客户机

/* the notify function used when creating a virt queue */
vp_notify               
    iowrite16(vq->index, vp_dev->ioaddr + VIRTIO_PCI_QUEUE_NOTIFY);
    
/* Handle a configuration change: Tell driver if it wants to know. */
vp_config_changed
    drv->config_changed
    virtio_blk  --  .config_changed     = virtblk_config_changed, -- queue_work
    virtio_console -- .config_changed = config_intr, -- 
    virtio_net_driver -- .config_changed = virtnet_config_changed, -- schedule_work
    
    
/* Notify all virtqueues on an interrupt. */
vp_vring_interrupt
    vring_interrupt

/* A small wrapper to also acknowledge the interrupt when it's handled. */
vp_interrupt
    vp_config_changed
    vp_vring_interrupt
    
virtblk_probe
    INIT_WORK(&vblk->config_work, virtblk_config_changed_work);
    



