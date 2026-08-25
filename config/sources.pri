# 目录重组后的 qmake 清单。保留已有裸 include，通过 INCLUDEPATH 保持源码内容不变。

INCLUDEPATH += \
    $$PWD/../src/app \
    $$PWD/../src/ui/shell \
    $$PWD/../src/core/network \
    $$PWD/../src/core/persistence \
    $$PWD/../src/features/nodes \
    $$PWD/../src/features/directory \
    $$PWD/../src/features/transfers

SOURCES += \
    $$PWD/../src/app/main.cpp \
    $$PWD/../src/ui/shell/HomeDownloadSelectionPolicy.cpp \
    $$PWD/../src/ui/shell/HomeFileRowPresentation.cpp \
    $$PWD/../src/ui/shell/HomeTaskErrorPolicy.cpp \
    $$PWD/../src/ui/shell/HomeWidge.cpp \
    $$PWD/../src/ui/shell/HomeWidgeComposition.cpp \
    $$PWD/../src/ui/shell/HomeWidgeRuntimeFeedback.cpp \
    $$PWD/../src/ui/shell/TitleBar.cpp \
    $$PWD/../src/core/network/NetWork.cpp \
    $$PWD/../src/core/network/DirectoryGateway.cpp \
    $$PWD/../src/core/network/NodeGateway.cpp \
    $$PWD/../src/core/network/NodeService.cpp \
    $$PWD/../src/core/network/DirectoryService.cpp \
    $$PWD/../src/core/network/TransferService.cpp \
    $$PWD/../src/core/network/TransferServiceFileTransfer.cpp \
    $$PWD/../src/core/network/TransferProtocolClient.cpp \
    $$PWD/../src/core/network/TransferControlState.cpp \
    $$PWD/../src/core/persistence/Database.cpp \
    $$PWD/../src/features/nodes/NodeModule.cpp \
    $$PWD/../src/features/nodes/NodePageController.cpp \
    $$PWD/../src/features/nodes/NodeDialog.cpp \
    $$PWD/../src/features/nodes/NodeItem.cpp \
    $$PWD/../src/features/directory/DirectoryModule.cpp \
    $$PWD/../src/features/directory/DirectoryNavigator.cpp \
    $$PWD/../src/features/directory/DirectoryPathNavigation.cpp \
    $$PWD/../src/features/directory/DirectoryLoadCoordinator.cpp \
    $$PWD/../src/features/directory/DirectoryTabPresentation.cpp \
    $$PWD/../src/features/directory/DirectoryPageController.cpp \
    $$PWD/../src/features/directory/DirectoryPageControllerDialogs.cpp \
    $$PWD/../src/features/directory/DirectoryPageDialog.cpp \
    $$PWD/../src/features/directory/DirectorySelectionDialog.cpp \
    $$PWD/../src/features/directory/DirectoryTabPage.cpp \
    $$PWD/../src/features/directory/FileBrowser.cpp \
    $$PWD/../src/features/directory/FileItem.cpp \
    $$PWD/../src/features/transfers/TaskManager.cpp \
    $$PWD/../src/features/transfers/TaskFeedbackSummary.cpp \
    $$PWD/../src/features/transfers/TaskCreationGateway.cpp \
    $$PWD/../src/features/transfers/TaskTransferGateway.cpp \
    $$PWD/../src/features/transfers/TaskNodeNameGateway.cpp \
    $$PWD/../src/features/transfers/TaskModule.cpp \
    $$PWD/../src/features/transfers/TaskModuleScheduling.cpp \
    $$PWD/../src/features/transfers/TaskListController.cpp \
    $$PWD/../src/features/transfers/TaskItem.cpp \
    $$PWD/../src/features/transfers/UploadController.cpp \
    $$PWD/../src/features/transfers/UploadModule.cpp \
    $$PWD/../src/features/transfers/DownloadController.cpp \
    $$PWD/../src/features/transfers/DownloadModule.cpp

HEADERS += \
    $$PWD/../src/ui/shell/HomeDownloadSelectionPolicy.h \
    $$PWD/../src/ui/shell/HomeFileRowPresentation.h \
    $$PWD/../src/ui/shell/HomeTaskErrorPolicy.h \
    $$PWD/../src/ui/shell/HomeTaskStatusPolicy.h \
    $$PWD/../src/ui/shell/HomeWidge.h \
    $$PWD/../src/ui/shell/TitleBar.h \
    $$PWD/../src/core/network/NetWork.h \
    $$PWD/../src/core/network/DirectoryGateway.h \
    $$PWD/../src/core/network/NetworkTypes.h \
    $$PWD/../src/core/network/TransferTypes.h \
    $$PWD/../src/core/network/NodeGateway.h \
    $$PWD/../src/core/network/NodeService.h \
    $$PWD/../src/core/network/DirectoryService.h \
    $$PWD/../src/core/network/TransferService.h \
    $$PWD/../src/core/network/TransferProtocolClient.h \
    $$PWD/../src/core/network/TransferControlState.h \
    $$PWD/../src/core/persistence/Database.h \
    $$PWD/../src/features/nodes/NodeModule.h \
    $$PWD/../src/features/nodes/NodePageController.h \
    $$PWD/../src/features/nodes/NodeDialog.h \
    $$PWD/../src/features/nodes/NodeItem.h \
    $$PWD/../src/features/directory/DirectoryModule.h \
    $$PWD/../src/features/directory/DirectoryNavigator.h \
    $$PWD/../src/features/directory/FileTypePolicy.h \
    $$PWD/../src/features/directory/DirectoryPathNavigation.h \
    $$PWD/../src/features/directory/DirectorySelectionPolicy.h \
    $$PWD/../src/features/directory/DirectoryLoadCoordinator.h \
    $$PWD/../src/features/directory/DirectoryTabPresentation.h \
    $$PWD/../src/features/directory/DirectoryPageController.h \
    $$PWD/../src/features/directory/DirectoryPageDialog.h \
    $$PWD/../src/features/directory/DirectorySelectionDialog.h \
    $$PWD/../src/features/directory/DirectoryTabPage.h \
    $$PWD/../src/features/directory/FileBrowser.h \
    $$PWD/../src/features/directory/FileItem.h \
    $$PWD/../src/features/transfers/TaskManager.h \
    $$PWD/../src/features/transfers/TaskPresentationPolicy.h \
    $$PWD/../src/features/transfers/TaskFeedbackSummary.h \
    $$PWD/../src/features/transfers/TaskCreationGateway.h \
    $$PWD/../src/features/transfers/TaskTransferGateway.h \
    $$PWD/../src/features/transfers/TaskNodeNameGateway.h \
    $$PWD/../src/features/transfers/TaskModule.h \
    $$PWD/../src/features/transfers/TaskListController.h \
    $$PWD/../src/features/transfers/TaskItem.h \
    $$PWD/../src/features/transfers/UploadController.h \
    $$PWD/../src/features/transfers/UploadModule.h \
    $$PWD/../src/features/transfers/DownloadController.h \
    $$PWD/../src/features/transfers/DownloadModule.h

FORMS += \
    $$PWD/../src/ui/shell/HomeWidge.ui \
    $$PWD/../src/ui/shell/TitleBar.ui \
    $$PWD/../src/features/nodes/NodeDialog.ui \
    $$PWD/../src/features/nodes/NodeItem.ui \
    $$PWD/../src/features/directory/DirectoryPageDialog.ui \
    $$PWD/../src/features/directory/DirectorySelectionDialog.ui \
    $$PWD/../src/features/directory/DirectoryTabPage.ui \
    $$PWD/../src/features/directory/FileItem.ui \
    $$PWD/../src/features/transfers/TaskItem.ui
