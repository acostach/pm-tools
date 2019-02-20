FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SUMMARY = "Small daemon used for restarting wifi/bluetooth after system suspend"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

DEPENDS += "glib-2.0"

inherit pkgconfig systemd

SRC_URI = " file://Makefile \
	file://pm.cpp \
	file://pm-daemon.service \
"

S = "${WORKDIR}"

do_install() {
	install -d ${D}${bindir}
	install -d ${D}/${systemd_unitdir}/system
	install -m 0755 pm-daemon ${D}${bindir}
	install -m 0755 ${WORKDIR}/pm-daemon.service ${D}/${systemd_unitdir}/system/pm-daemon.service
}

SYSTEMD_SERVICE_${PN} = "pm-daemon.service"
