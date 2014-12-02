/* rockchip_drm_iommu.h
 *
 * Copyright (c) 2014 Romain Perier <romain.perier@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _ROCKCHIP_DRM_IOMMU_H_
#define _ROCKCHIP_DRM_IOMMU_H_

#ifdef CONFIG_DRM_ROCKCHIP_IOMMU

int rockchip_drm_iommu_create_mapping(struct drm_device *drm_dev);

void rockchip_drm_iommu_release_mapping(struct drm_device *drm_dev);

int rockchip_drm_iommu_attach_master(struct drm_device *drm_dev);

int rockchip_drm_iommu_attach_device(struct drm_device *drm_dev,
				struct device *subdrv_dev);

void rockchip_drm_iommu_detach_master(struct drm_device *drm_dev);

void rockchip_drm_iommu_detach_device(struct drm_device *drm_dev,
				struct device *subdrv_dev);

static inline bool rockchip_is_drm_iommu_supported(struct drm_device *drm_dev)
{
#ifdef CONFIG_ARM_DMA_USE_IOMMU
	struct device *dev = drm_dev->dev;

	return dev->archdata.mapping ? true : false;
#else
	return false;
#endif
}

#else

struct dma_iommu_mapping;
static inline int rockchip_drm_iommu_create_mapping(struct drm_device *drm_dev)
{
	return 0;
}

static inline void rockchip_drm_iommu_release_mapping(struct drm_device *drm_dev)
{
}

static inline int rockchip_drm_iommu_attach_master(struct drm_device *drm_dev)
{
	return 0;
}

static inline int rockchip_drm_iommu_attach_device(struct drm_device *drm_dev,
						struct device *subdrv_dev)
{
	return 0;
}

static inline void rockchip_drm_iommu_detach_master(struct drm_device *drm_dev)
{
}

static inline void rockchip_drm_iommu_detach_device(struct drm_device *drm_dev,
						struct device *subdrv_dev)
{
}

static inline bool rockchip_is_drm_iommu_supported(struct drm_device *drm_dev)
{
	return false;
}

#endif
#endif
