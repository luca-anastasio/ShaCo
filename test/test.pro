TEMPLATE = subdirs
SUBDIRS = \
    portdiscovery \
    machineinfo \
    machinecommunication \
    gcodesender \
    testcommon \
    wirecontroller \
    machinestate \
    machinestatusmonitor \
    commandsender

portdiscovery.depends = testcommon
machineinfo.depends = testcommon
machinecommunication.depends = testcommon
gcodesender.depends = testcommon
wirecontroller.depends = testcommon
