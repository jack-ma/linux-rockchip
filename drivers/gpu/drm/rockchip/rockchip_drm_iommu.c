/* rockchip_drm_iommu.c
 *
 * Copyright (c) 2014 Romain Perier <romain.perier@gmail.com>
 *
 * greatly inpired from exynos_drm_iommu.c
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <drmP.h>

#include <linux/dma-mapping.h>
#include <linux/iommu.h>
#include <linux/kref.h>

#include <asm/dma-iommu.h>

/*
 * rockchip_drm_iommu_create_mapping - create a mapping structure
 *
 * @drm_dev: DRM device
 */
int rockchip_drm_iommu_create_mapping(struct drm_device *drm_dev)
{
	struct dma_iommu_mapping *mapping = NULL;
	struct device *dev = drm_dev->dev;
	int ret;

	mapping = arm_iommu_create_mapping(&platform_bus_type, 0x00000000,
					SZ_2G);
	if (IS_ERR(mapping))
		return PTR_ERR(mapping);

	dev->dma_parms = devm_kzalloc(dev, sizeof(*dev->dma_parms), GFP_KERNEL);

	if (!dev->dma_parms) {
		ret = -ENOMEM;
		goto error;
	}

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		goto error;

	ret = dma_set_max_seg_size(dev, DMA_BIT_MASK(32));
	if (ret)
		goto error;

	dev->archdata.mapping = mapping;

	return 0;
error:
	arm_iommu_release_mapping(mapping);
	return ret;
}

/*
 * rockchip_drm_iommu_release_mapping - release iommu mapping structure
 *
 * @drm_dev: DRM device
 *
 * if mapping->kref becomes 0 then all things related to iommu mapping
 * will be released
 */
void rockchip_drm_iommu_release_mapping(struct drm_device *drm_dev)
{
	arm_iommu_release_mapping(drm_dev->dev->archdata.mapping);
}

/*
 * rockchip_drm_iommu_attach_master - attach DRM master device to iommu mapping
 *
 * @drm_dev: DRM device
 */
int rockchip_drm_iommu_attach_master(struct drm_device *drm_dev)
{
	struct device *dev = drm_dev->dev;

	return arm_iommu_attach_device(dev, dev->archdata.mapping);
}

/*
 * rockchip_drm_iommu_attach_device - attach device to iommu mapping
 *
 * @drm_dev: DRM device
 * @subdrv_dev: device to be attach
 *
 * This function should be called by sub drivers to attach it to iommu
 * mapping.
 */
int rockchip_drm_iommu_attach_device(struct drm_device *drm_dev,
			struct device *subdrv_dev)
{
	struct device *dev = drm_dev->dev;
	int ret;

	if (!dev->archdata.mapping) {
		DRM_ERROR("iommu_mapping is null.\n");
		return -EFAULT;
	}

	subdrv_dev->dma_parms = devm_kzalloc(subdrv_dev,
					sizeof(*subdrv_dev->dma_parms),
					GFP_KERNEL);
	if (!subdrv_dev->dma_parms)
		return -ENOMEM;

	ret = dma_set_coherent_mask(subdrv_dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	dma_set_max_seg_size(subdrv_dev, DMA_BIT_MASK(32));

	ret = arm_iommu_attach_device(subdrv_dev, dev->archdata.mapping);
	if (ret < 0) {
		DRM_DEBUG_KMS("failed iommu attach.\n");
		return ret;
	}

	/*
	 * Set dma_ops to drm_device just one time.
	 *
	 * The dma mapping api needs device object and the api is used
	 * to allocate physical memory and map it with iommu table.
	 * If iommu attach succeeded, the sub driver would have dma_ops
	 * for iommu and also all sub drivers have same dma_ops.
	 */
	if (!dev->archdata.dma_ops)
		dev->archdata.dma_ops = subdrv_dev->archdata.dma_ops;

	return 0;
}


void rockchip_drm_iommu_detach_master(struct drm_device *drm_dev)
{
	arm_iommu_detach_device(drm_dev->dev);
}

/*
 * rockchip_drm_iommu_detach_device - detach device address space mapping from device
 *
 * @drm_dev: DRM device
 * @subdrv_dev: device to be detached
 *
 * This function should be called by sub drivers to detach it from iommu
 * mapping
 */
void rockchip_drm_iommu_detach_device(struct drm_device *drm_dev,
				struct device *subdrv_dev)
{
	arm_iommu_detach_device(subdrv_dev);
}
