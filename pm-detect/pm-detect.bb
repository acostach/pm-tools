FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
DESCRIPTION = "pm-detect module"
PR = "r0"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

inherit module

DEPENDS += "make-mod-scripts"

EXTRA_OEMAKE_append = " KSRC=${STAGING_KERNEL_DIR}"

SRC_URI = " file://pm_detect.c \
	file://Makefile \
"

S = "${WORKDIR}"

KERNEL_MODULE_AUTOLOAD += "pm_detect"
KERNEL_MODULE_PROBECONF += "pm_detect"


