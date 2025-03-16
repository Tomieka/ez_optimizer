#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtCore/QProcess>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressDialog>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QStatusBar>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>
#include <unistd.h>
#include <QTemporaryFile>

class CacheManagementWidget : public QWidget
{
    Q_OBJECT

public:
    CacheManagementWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QLabel *infoLabel = new QLabel("This tab helps you manage the Pacman cache.\nThe cache is located at /var/cache/pacman/pkg/", this);
        mainLayout->addWidget(infoLabel);

        QHBoxLayout *sizeLayout = new QHBoxLayout();
        QLabel *sizeLabel = new QLabel("Current cache size:", this);
        m_sizeValueLabel = new QLabel("Calculating...", this);
        sizeLayout->addWidget(sizeLabel);
        sizeLayout->addWidget(m_sizeValueLabel);
        sizeLayout->addStretch();
        mainLayout->addLayout(sizeLayout);

        QHBoxLayout *cacheButtonLayout = new QHBoxLayout();
        
        m_refreshButton = new QPushButton("Refresh", this);
        connect(m_refreshButton, &QPushButton::clicked, this, &CacheManagementWidget::refreshCacheSize);
        
        m_clearButton = new QPushButton("Clear Cache", this);
        connect(m_clearButton, &QPushButton::clicked, this, &CacheManagementWidget::clearCache);

        cacheButtonLayout->addWidget(m_refreshButton);
        cacheButtonLayout->addWidget(m_clearButton);
        
        mainLayout->addLayout(cacheButtonLayout);
        mainLayout->addStretch();

        m_statusLabel = new QLabel("Ready", this);
        mainLayout->addWidget(m_statusLabel);

        refreshCacheSize();
    }

public slots:
    void refreshCacheSize()
    {
        m_statusLabel->setText("Calculating cache size...");
        m_refreshButton->setEnabled(false);
        m_clearButton->setEnabled(false);

        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                m_refreshButton->setEnabled(true);
                m_clearButton->setEnabled(true);
                
                if (exitCode == 0) {
                    QString output = process->readAllStandardOutput();
                    m_sizeValueLabel->setText(output.trimmed());
                    m_statusLabel->setText("Ready");
                } else {
                    m_sizeValueLabel->setText("Error");
                    m_statusLabel->setText("Failed to calculate cache size");
                }
                
                process->deleteLater();
            });

        process->start("bash", QStringList() << "-c" << "du -sh /var/cache/pacman/pkg/ | cut -f1");
    }

    void clearCache()
    {
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            "Confirm Cache Clearing", 
            "Are you sure you want to clear the Pacman cache?",
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::No)
            return;

        m_statusLabel->setText("Clearing cache...");
        m_refreshButton->setEnabled(false);
        m_clearButton->setEnabled(false);
        
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                m_refreshButton->setEnabled(true);
                m_clearButton->setEnabled(true);
                
                if (exitCode == 0) {
                    m_statusLabel->setText("Cache cleared successfully");
                    QMessageBox::information(this, "Success", "Pacman cache cleared successfully");
                    refreshCacheSize();
                } else {
                    QString error = process->readAllStandardError();
                    m_statusLabel->setText("Failed to clear cache");
                    QMessageBox::critical(this, "Error", "Failed to clear Pacman cache.\n" + error);
                }
                
                process->deleteLater();
            });

        process->start("bash", QStringList() << "-c" << "rm -rf /var/cache/pacman/pkg/*");
    }

private:
    QLabel *m_sizeValueLabel;
    QLabel *m_statusLabel;
    QPushButton *m_refreshButton;
    QPushButton *m_clearButton;
};

class OrphanedPackagesWidget : public QWidget
{
    Q_OBJECT

public:
    OrphanedPackagesWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QLabel *infoLabel = new QLabel("This tab helps you manage orphaned packages.\nOrphaned packages are packages installed as dependencies but no longer needed.", this);
        mainLayout->addWidget(infoLabel);
        
        QLabel *orphanedLabel = new QLabel("Orphaned packages:", this);
        mainLayout->addWidget(orphanedLabel);
        
        m_selectAllCheckBox = new QCheckBox("Select All", this);
        connect(m_selectAllCheckBox, &QCheckBox::stateChanged, this, &OrphanedPackagesWidget::onSelectAllChanged);
        mainLayout->addWidget(m_selectAllCheckBox);
        
        m_orphanedPackagesList = new QListWidget(this);
        m_orphanedPackagesList->setSelectionMode(QAbstractItemView::NoSelection);
        m_orphanedPackagesList->setMaximumHeight(200);
        mainLayout->addWidget(m_orphanedPackagesList);
        
        QHBoxLayout *orphanedButtonLayout = new QHBoxLayout();
        
        m_listOrphansButton = new QPushButton("List Orphaned Packages", this);
        connect(m_listOrphansButton, &QPushButton::clicked, this, &OrphanedPackagesWidget::listOrphanedPackages);
        
        m_removeOrphansButton = new QPushButton("Remove Selected Packages", this);
        connect(m_removeOrphansButton, &QPushButton::clicked, this, &OrphanedPackagesWidget::removeOrphanedPackages);
        m_removeOrphansButton->setEnabled(false);
        
        orphanedButtonLayout->addWidget(m_listOrphansButton);
        orphanedButtonLayout->addWidget(m_removeOrphansButton);
        
        mainLayout->addLayout(orphanedButtonLayout);
        
        mainLayout->addStretch();

        m_statusLabel = new QLabel("Ready", this);
        mainLayout->addWidget(m_statusLabel);
    }

public slots:
    void onSelectAllChanged(int state)
    {
        Qt::CheckState checkState = static_cast<Qt::CheckState>(state);
        for (int i = 0; i < m_orphanedPackagesList->count(); i++) {
            QListWidgetItem* item = m_orphanedPackagesList->item(i);
            item->setCheckState(checkState);
        }
        
        updateRemoveButtonState();
    }
    
    void listOrphanedPackages()
    {
        m_statusLabel->setText("Listing orphaned packages...");
        m_listOrphansButton->setEnabled(false);
        m_removeOrphansButton->setEnabled(false);
        m_orphanedPackagesList->clear();
        m_selectAllCheckBox->setChecked(false);
        m_selectAllCheckBox->setEnabled(false);

        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                m_listOrphansButton->setEnabled(true);
                
                if (exitCode == 0) {
                    QString output = process->readAllStandardOutput();
                    QStringList packages = output.split('\n', Qt::SkipEmptyParts);
                    
                    if (packages.isEmpty() || (packages.size() == 1 && packages[0].contains("No orphaned packages found"))) {
                        QListWidgetItem* item = new QListWidgetItem("No orphaned packages found");
                        item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
                        m_orphanedPackagesList->addItem(item);
                        
                        m_statusLabel->setText("No orphaned packages found");
                        m_removeOrphansButton->setEnabled(false);
                        m_selectAllCheckBox->setEnabled(false);
                    } else {
                        for (const QString& pkg : packages) {
                            QListWidgetItem* item = new QListWidgetItem(pkg.trimmed());
                            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                            item->setCheckState(Qt::Unchecked);
                            m_orphanedPackagesList->addItem(item);
                            
                            connect(m_orphanedPackagesList, &QListWidget::itemChanged, this, &OrphanedPackagesWidget::updateRemoveButtonState);
                        }
                        
                        m_statusLabel->setText(QString("Found %1 orphaned packages").arg(packages.size()));
                        m_selectAllCheckBox->setEnabled(true);
                        updateRemoveButtonState();
                    }
                } else {
                    QString error = process->readAllStandardError();
                    QListWidgetItem* item = new QListWidgetItem("Error listing orphaned packages: " + error);
                    item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
                    m_orphanedPackagesList->addItem(item);
                    
                    m_statusLabel->setText("Error listing orphaned packages");
                    m_removeOrphansButton->setEnabled(false);
                    m_selectAllCheckBox->setEnabled(false);
                }
                
                process->deleteLater();
            });

        process->start("bash", QStringList() << "-c" << "pacman -Qdt 2>/dev/null || echo 'No orphaned packages found'");
    }
    
    void updateRemoveButtonState()
    {
        bool anyChecked = false;
        for (int i = 0; i < m_orphanedPackagesList->count(); i++) {
            QListWidgetItem* item = m_orphanedPackagesList->item(i);
            if (item->flags() & Qt::ItemIsUserCheckable && item->checkState() == Qt::Checked) {
                anyChecked = true;
                break;
            }
        }
        
        m_removeOrphansButton->setEnabled(anyChecked);
    }
    
    void removeOrphanedPackages()
    {
        QStringList packagesToRemove;
        for (int i = 0; i < m_orphanedPackagesList->count(); i++) {
            QListWidgetItem* item = m_orphanedPackagesList->item(i);
            if (item->flags() & Qt::ItemIsUserCheckable && item->checkState() == Qt::Checked) {
                QString pkgName = item->text().section(' ', 0, 0);
                packagesToRemove << pkgName;
            }
        }
        
        if (packagesToRemove.isEmpty()) {
            return;
        }
        
        QString packageList = packagesToRemove.join(", ");
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            "Confirm Package Removal", 
            QString("Are you sure you want to remove these orphaned packages?\n%1").arg(packageList),
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::No)
            return;

        m_statusLabel->setText("Removing selected orphaned packages...");
        m_listOrphansButton->setEnabled(false);
        m_removeOrphansButton->setEnabled(false);
        m_selectAllCheckBox->setEnabled(false);

        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                m_listOrphansButton->setEnabled(true);
                m_selectAllCheckBox->setEnabled(true);
                
                if (exitCode == 0) {
                    m_statusLabel->setText("Selected packages removed successfully");
                    QMessageBox::information(this, "Success", "Selected orphaned packages removed successfully");
                    listOrphanedPackages();
                } else {
                    QString error = process->readAllStandardError();
                    m_statusLabel->setText("Failed to remove selected packages");
                    QMessageBox::critical(this, "Error", "Failed to remove orphaned packages.\n" + error);
                    updateRemoveButtonState();
                }
                
                process->deleteLater();
            });

        QString packagesArg = packagesToRemove.join(" ");
        
        process->start("bash", QStringList() << "-c" << QString("pacman -Rns %1 --noconfirm").arg(packagesArg));
    }

private:
    QLabel *m_statusLabel;
    QPushButton *m_listOrphansButton;
    QPushButton *m_removeOrphansButton;
    QListWidget *m_orphanedPackagesList;
    QCheckBox *m_selectAllCheckBox;
};

class SystemLogsWidget : public QWidget
{
    Q_OBJECT

public:
    SystemLogsWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QLabel *infoLabel = new QLabel("This tab helps you manage system logs.\nYou can view, compress, or remove old log files to save disk space.", this);
        mainLayout->addWidget(infoLabel);
        
        m_logsTable = new QTableWidget(0, 3, this);
        m_logsTable->setHorizontalHeaderLabels(QStringList() << "Log File" << "Size" << "Last Modified");
        m_logsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_logsTable->setSelectionMode(QAbstractItemView::MultiSelection);
        m_logsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_logsTable->setMinimumHeight(200);
        mainLayout->addWidget(m_logsTable);
        
        QGroupBox *actionsGroupBox = new QGroupBox("Log Management Actions", this);
        QVBoxLayout *actionsLayout = new QVBoxLayout(actionsGroupBox);
        
        QHBoxLayout *ageLayout = new QHBoxLayout();
        QLabel *ageLabel = new QLabel("Process logs older than:", this);
        m_ageSpinBox = new QSpinBox(this);
        m_ageSpinBox->setMinimum(1);
        m_ageSpinBox->setMaximum(365);
        m_ageSpinBox->setValue(30);
        m_ageUnitCombo = new QComboBox(this);
        m_ageUnitCombo->addItems(QStringList() << "Days" << "Weeks" << "Months");
        ageLayout->addWidget(ageLabel);
        ageLayout->addWidget(m_ageSpinBox);
        ageLayout->addWidget(m_ageUnitCombo);
        ageLayout->addStretch();
        actionsLayout->addLayout(ageLayout);
        
        QButtonGroup *actionGroup = new QButtonGroup(this);
        
        m_compressLogsRadio = new QRadioButton("Compress selected logs", this);
        m_removeLogsRadio = new QRadioButton("Remove selected logs", this);
        m_compressLogsRadio->setChecked(true);
        
        actionGroup->addButton(m_compressLogsRadio);
        actionGroup->addButton(m_removeLogsRadio);
        
        actionsLayout->addWidget(m_compressLogsRadio);
        actionsLayout->addWidget(m_removeLogsRadio);
        
        mainLayout->addWidget(actionsGroupBox);
        
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        
        m_refreshLogsButton = new QPushButton("Refresh Logs List", this);
        connect(m_refreshLogsButton, &QPushButton::clicked, this, &SystemLogsWidget::refreshLogsList);
        
        m_selectOldLogsButton = new QPushButton("Select Old Logs", this);
        connect(m_selectOldLogsButton, &QPushButton::clicked, this, &SystemLogsWidget::selectOldLogs);
        
        m_processLogsButton = new QPushButton("Process Selected Logs", this);
        connect(m_processLogsButton, &QPushButton::clicked, this, &SystemLogsWidget::processLogs);
        m_processLogsButton->setEnabled(false);
        
        buttonLayout->addWidget(m_refreshLogsButton);
        buttonLayout->addWidget(m_selectOldLogsButton);
        buttonLayout->addWidget(m_processLogsButton);
        
        mainLayout->addLayout(buttonLayout);
        
        m_statusLabel = new QLabel("Ready", this);
        mainLayout->addWidget(m_statusLabel);
        
        connect(m_logsTable, &QTableWidget::itemSelectionChanged, this, &SystemLogsWidget::updateButtonState);
        
        refreshLogsList();
    }

public slots:
    void refreshLogsList()
    {
        m_statusLabel->setText("Refreshing logs list...");
        m_refreshLogsButton->setEnabled(false);
        m_selectOldLogsButton->setEnabled(false);
        m_processLogsButton->setEnabled(false);
        m_logsTable->clearContents();
        m_logsTable->setRowCount(0);

        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                m_refreshLogsButton->setEnabled(true);
                m_selectOldLogsButton->setEnabled(true);
                
                if (exitCode == 0) {
                    QString output = process->readAllStandardOutput();
                    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
                    
                    m_logsTable->setRowCount(lines.size());
                    
                    int row = 0;
                    for (const QString &line : lines) {
                        QStringList parts = line.split('|');
                        if (parts.size() >= 3) {
                            QTableWidgetItem *fileItem = new QTableWidgetItem(parts[0].trimmed());
                            QTableWidgetItem *sizeItem = new QTableWidgetItem(parts[1].trimmed());
                            QTableWidgetItem *dateItem = new QTableWidgetItem(parts[2].trimmed());
                            
                            fileItem->setData(Qt::UserRole, parts[0].trimmed());
                            
                            m_logsTable->setItem(row, 0, fileItem);
                            m_logsTable->setItem(row, 1, sizeItem);
                            m_logsTable->setItem(row, 2, dateItem);
                            row++;
                        }
                    }
                    
                    m_statusLabel->setText(QString("Found %1 log files").arg(lines.size()));
                } else {
                    m_statusLabel->setText("Failed to get logs list");
                }
                
                updateButtonState();
                process->deleteLater();
            });

        process->start("bash", QStringList() << "-c" << 
            "find /var/log -type f -name '*.log' -o -name '*.log.[0-9]*' | "
            "while read file; do "
            "  echo \"$file|$(du -h \"$file\" | cut -f1)|$(date -r \"$file\" '+%Y-%m-%d %H:%M')\"; "
            "done");
    }
    
    void selectOldLogs()
    {
        int age = m_ageSpinBox->value();
        QString unit = m_ageUnitCombo->currentText();
        
        int days = age;
        if (unit == "Weeks") {
            days = age * 7;
        } else if (unit == "Months") {
            days = age * 30;
        }
        
        m_statusLabel->setText(QString("Selecting logs older than %1 %2...").arg(age).arg(unit.toLower()));
        
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode == 0) {
                    QString output = process->readAllStandardOutput();
                    QStringList oldFiles = output.split('\n', Qt::SkipEmptyParts);
                    
                    m_logsTable->clearSelection();
                    
                    int selectedCount = 0;
                    for (int row = 0; row < m_logsTable->rowCount(); row++) {
                        QTableWidgetItem *item = m_logsTable->item(row, 0);
                        if (item) {
                            QString filePath = item->data(Qt::UserRole).toString();
                            if (oldFiles.contains(filePath)) {
                                m_logsTable->selectRow(row);
                                selectedCount++;
                            }
                        }
                    }
                    
                    m_statusLabel->setText(QString("Selected %1 log files older than %2 %3")
                        .arg(selectedCount).arg(age).arg(unit.toLower()));
                } else {
                    m_statusLabel->setText("Failed to select old logs");
                }
                
                updateButtonState();
                process->deleteLater();
            });
        
        process->start("bash", QStringList() << "-c" << 
            QString("find /var/log -type f -mtime +%1 -name '*.log' -o -name '*.log.[0-9]*'").arg(days));
    }
    
    void processLogs()
    {
        QList<QTableWidgetItem*> selectedItems = m_logsTable->selectedItems();
        QStringList selectedFiles;
        
        QSet<int> selectedRows;
        for (QTableWidgetItem* item : selectedItems) {
            selectedRows.insert(item->row());
        }
        
        for (int row : selectedRows) {
            QTableWidgetItem* item = m_logsTable->item(row, 0);
            if (item) {
                selectedFiles << item->data(Qt::UserRole).toString();
            }
        }
        
        if (selectedFiles.isEmpty()) {
            m_statusLabel->setText("No log files selected");
            return;
        }
        
        bool compress = m_compressLogsRadio->isChecked();
        QString operation = compress ? "compress" : "remove";
        
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            QString("Confirm Log %1").arg(operation.at(0).toUpper() + operation.mid(1)),
            QString("Are you sure you want to %1 %2 selected log files?\nThis requires administrator privileges.")
                .arg(operation).arg(selectedFiles.size()),
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::No)
            return;
        
        m_statusLabel->setText(QString("%1 selected log files...").arg(operation.at(0).toUpper() + operation.mid(1)));
        m_refreshLogsButton->setEnabled(false);
        m_selectOldLogsButton->setEnabled(false);
        m_processLogsButton->setEnabled(false);
        
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                m_refreshLogsButton->setEnabled(true);
                m_selectOldLogsButton->setEnabled(true);
                
                if (exitCode == 0) {
                    QString actionVerb = compress ? "compressed" : "removed";
                    m_statusLabel->setText(QString("Successfully %1 selected log files").arg(actionVerb));
                    QMessageBox::information(this, "Success", 
                        QString("Successfully %1 %2 log files").arg(actionVerb).arg(selectedFiles.size()));
                    refreshLogsList();
                } else {
                    QString error = process->readAllStandardError();
                    m_statusLabel->setText(QString("Failed to %1 log files").arg(operation));
                    QMessageBox::critical(this, "Error", 
                        QString("Failed to %1 log files.\n%2").arg(operation).arg(error));
                    updateButtonState();
                }
                
                process->deleteLater();
            });
        
        QString script;
        if (compress) {
            script = "for file in " + selectedFiles.join(" ") + "; do\n"
                     "  if [[ \"$file\" != *.gz ]]; then\n"
                     "    gzip -f \"$file\"\n"
                     "  fi\n"
                     "done";
        } else {
            script = "rm -f " + selectedFiles.join(" ");
        }
        
        process->start("pkexec", QStringList() << "bash" << "-c" << script);
    }
    
    void updateButtonState()
    {
        m_processLogsButton->setEnabled(m_logsTable->selectedItems().size() > 0);
    }

private:
    QTableWidget *m_logsTable;
    QLabel *m_statusLabel;
    QPushButton *m_refreshLogsButton;
    QPushButton *m_selectOldLogsButton;
    QPushButton *m_processLogsButton;
    QRadioButton *m_compressLogsRadio;
    QRadioButton *m_removeLogsRadio;
    QSpinBox *m_ageSpinBox;
    QComboBox *m_ageUnitCombo;
};

class SystemServicesWidget : public QWidget
{
    Q_OBJECT

public:
    SystemServicesWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QLabel *infoLabel = new QLabel("This tab helps you manage system services.\nYou can view running services and enable/disable them at startup.", this);
        mainLayout->addWidget(infoLabel);
        
        QHBoxLayout *searchLayout = new QHBoxLayout();
        QLabel *searchLabel = new QLabel("Filter services:", this);
        m_searchEdit = new QLineEdit(this);
        m_searchEdit->setPlaceholderText("Enter service name to filter...");
        connect(m_searchEdit, &QLineEdit::textChanged, this, &SystemServicesWidget::filterServices);
        
        searchLayout->addWidget(searchLabel);
        searchLayout->addWidget(m_searchEdit);
        mainLayout->addLayout(searchLayout);
        
        m_servicesTable = new QTableWidget(0, 4, this);
        m_servicesTable->setHorizontalHeaderLabels(QStringList() << "Service Name" << "Description" << "Status" << "Startup");
        m_servicesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_servicesTable->setSelectionMode(QAbstractItemView::SingleSelection);
        m_servicesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_servicesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_servicesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_servicesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        m_servicesTable->setMinimumHeight(300);
        mainLayout->addWidget(m_servicesTable);
        
        connect(m_servicesTable, &QTableWidget::itemSelectionChanged, this, &SystemServicesWidget::updateButtonStates);
        
        QHBoxLayout *actionLayout = new QHBoxLayout();
        
        m_refreshButton = new QPushButton("Refresh List", this);
        connect(m_refreshButton, &QPushButton::clicked, this, &SystemServicesWidget::refreshServicesList);
        
        m_startButton = new QPushButton("Start Service", this);
        connect(m_startButton, &QPushButton::clicked, this, &SystemServicesWidget::startService);
        m_startButton->setEnabled(false);
        
        m_stopButton = new QPushButton("Stop Service", this);
        connect(m_stopButton, &QPushButton::clicked, this, &SystemServicesWidget::stopService);
        m_stopButton->setEnabled(false);
        
        m_enableButton = new QPushButton("Enable at Startup", this);
        connect(m_enableButton, &QPushButton::clicked, this, &SystemServicesWidget::enableService);
        m_enableButton->setEnabled(false);
        
        m_disableButton = new QPushButton("Disable at Startup", this);
        connect(m_disableButton, &QPushButton::clicked, this, &SystemServicesWidget::disableService);
        m_disableButton->setEnabled(false);
        
        actionLayout->addWidget(m_refreshButton);
        actionLayout->addWidget(m_startButton);
        actionLayout->addWidget(m_stopButton);
        actionLayout->addWidget(m_enableButton);
        actionLayout->addWidget(m_disableButton);
        
        mainLayout->addLayout(actionLayout);
        
        m_statusLabel = new QLabel("Ready", this);
        mainLayout->addWidget(m_statusLabel);
        
        refreshServicesList();
    }

public slots:
    void refreshServicesList()
    {
        m_statusLabel->setText("Fetching services list...");
        m_refreshButton->setEnabled(false);
        m_startButton->setEnabled(false);
        m_stopButton->setEnabled(false);
        m_enableButton->setEnabled(false);
        m_disableButton->setEnabled(false);
        m_searchEdit->setEnabled(false);
        
        m_servicesTable->clearContents();
        m_servicesTable->setRowCount(0);
        m_allServices.clear();

        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode == 0) {
                    QString output = process->readAllStandardOutput();
                    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
                    
                    m_allServices = lines;
                    
                    displayFilteredServices(m_allServices);
                    
                    m_statusLabel->setText(QString("Found %1 services").arg(m_allServices.size()));
                } else {
                    m_statusLabel->setText("Failed to get services list");
                }
                
                m_refreshButton->setEnabled(true);
                m_searchEdit->setEnabled(true);
                process->deleteLater();
            });

        process->start("bash", QStringList() << "-c" << 
            "systemctl list-units --type=service --all --no-legend | awk '{print $1}'");
    }
    
    void filterServices(const QString &filter)
    {
        if (m_allServices.isEmpty()) {
            return;
        }
        
        QStringList filteredServices;
        for (const QString &service : m_allServices) {
            if (service.contains(filter, Qt::CaseInsensitive)) {
                filteredServices << service;
            }
        }
        
        displayFilteredServices(filteredServices);
    }
    
    void displayFilteredServices(const QStringList &services)
    {
        m_servicesTable->clearContents();
        m_servicesTable->setRowCount(services.size());
        
        for (int i = 0; i < services.size(); ++i) {
            const QString &serviceName = services[i];
            
            fetchServiceDetails(i, serviceName);
        }
    }
    
    void fetchServiceDetails(int row, const QString &serviceName)
    {
        QTableWidgetItem *nameItem = new QTableWidgetItem(serviceName);
        m_servicesTable->setItem(row, 0, nameItem);
        
        QProcess *descProcess = new QProcess(this);
        connect(descProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                QString description = "No description available";
                if (exitCode == 0) {
                    description = descProcess->readAllStandardOutput().trimmed();
                    if (description.isEmpty()) {
                        description = "No description available";
                    }
                }
                QTableWidgetItem *descItem = new QTableWidgetItem(description);
                m_servicesTable->setItem(row, 1, descItem);
                descProcess->deleteLater();
            });
        
        descProcess->start("bash", QStringList() << "-c" << 
            QString("systemctl show %1 -p Description | sed 's/Description=//'").arg(serviceName));
        
        QProcess *activeProcess = new QProcess(this);
        connect(activeProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                QString status = "Unknown";
                if (exitCode == 0) {
                    status = activeProcess->readAllStandardOutput().trimmed();
                }
                QTableWidgetItem *statusItem = new QTableWidgetItem(status);
                
                if (status.contains("active", Qt::CaseInsensitive)) {
                    statusItem->setBackground(QColor(200, 255, 200));
                } else if (status.contains("inactive", Qt::CaseInsensitive)) {
                    statusItem->setBackground(QColor(255, 200, 200));
                }
                
                m_servicesTable->setItem(row, 2, statusItem);
                activeProcess->deleteLater();
            });
        
        activeProcess->start("bash", QStringList() << "-c" << 
            QString("systemctl is-active %1").arg(serviceName));
        
        QProcess *enabledProcess = new QProcess(this);
        connect(enabledProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                QString enabled = "Unknown";
                if (exitCode == 0) {
                    enabled = enabledProcess->readAllStandardOutput().trimmed();
                }
                QTableWidgetItem *enabledItem = new QTableWidgetItem(enabled);
                
                if (enabled.contains("enabled", Qt::CaseInsensitive)) {
                    enabledItem->setBackground(QColor(200, 255, 200));
                } else if (enabled.contains("disabled", Qt::CaseInsensitive)) {
                    enabledItem->setBackground(QColor(255, 200, 200));
                }
                
                m_servicesTable->setItem(row, 3, enabledItem);
                enabledProcess->deleteLater();
            });
        
        enabledProcess->start("bash", QStringList() << "-c" << 
            QString("systemctl is-enabled %1").arg(serviceName));
    }
    
    void updateButtonStates()
    {
        bool hasSelection = !m_servicesTable->selectedItems().isEmpty();
        m_startButton->setEnabled(hasSelection);
        m_stopButton->setEnabled(hasSelection);
        m_enableButton->setEnabled(hasSelection);
        m_disableButton->setEnabled(hasSelection);
    }
    
    void startService()
    {
        QList<QTableWidgetItem*> items = m_servicesTable->selectedItems();
        if (items.isEmpty()) {
            return;
        }
        
        int row = items.first()->row();
        QTableWidgetItem* nameItem = m_servicesTable->item(row, 0);
        if (!nameItem) {
            return;
        }
        
        QString serviceName = nameItem->text();
        
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            "Confirm Service Start", 
            QString("Are you sure you want to start the service '%1'?\nThis requires administrator privileges.").arg(serviceName),
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::No) {
            return;
        }
        
        m_statusLabel->setText(QString("Starting service %1...").arg(serviceName));
        setButtonsEnabled(false);
        
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                setButtonsEnabled(true);
                
                if (exitCode == 0) {
                    m_statusLabel->setText(QString("Service %1 started successfully").arg(serviceName));
                    fetchServiceDetails(row, serviceName);
                } else {
                    QString error = process->readAllStandardError();
                    m_statusLabel->setText(QString("Failed to start service %1").arg(serviceName));
                    QMessageBox::critical(this, "Error", 
                        QString("Failed to start service.\n%1").arg(error));
                }
                
                process->deleteLater();
            });
        
        process->start("pkexec", QStringList() << "systemctl" << "start" << serviceName);
    }
    
    void stopService()
    {
        QList<QTableWidgetItem*> items = m_servicesTable->selectedItems();
        if (items.isEmpty()) {
            return;
        }
        
        int row = items.first()->row();
        QTableWidgetItem* nameItem = m_servicesTable->item(row, 0);
        if (!nameItem) {
            return;
        }
        
        QString serviceName = nameItem->text();
        
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            "Confirm Service Stop", 
            QString("Are you sure you want to stop the service '%1'?\nThis requires administrator privileges.").arg(serviceName),
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::No) {
            return;
        }
        
        m_statusLabel->setText(QString("Stopping service %1...").arg(serviceName));
        setButtonsEnabled(false);
        
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                setButtonsEnabled(true);
                
                if (exitCode == 0) {
                    m_statusLabel->setText(QString("Service %1 stopped successfully").arg(serviceName));
                    fetchServiceDetails(row, serviceName);
                } else {
                    QString error = process->readAllStandardError();
                    m_statusLabel->setText(QString("Failed to stop service %1").arg(serviceName));
                    QMessageBox::critical(this, "Error", 
                        QString("Failed to stop service.\n%1").arg(error));
                }
                
                process->deleteLater();
            });
        
        process->start("pkexec", QStringList() << "systemctl" << "stop" << serviceName);
    }
    
    void enableService()
    {
        QList<QTableWidgetItem*> items = m_servicesTable->selectedItems();
        if (items.isEmpty()) {
            return;
        }
        
        int row = items.first()->row();
        QTableWidgetItem* nameItem = m_servicesTable->item(row, 0);
        if (!nameItem) {
            return;
        }
        
        QString serviceName = nameItem->text();
        
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            "Confirm Service Enable", 
            QString("Are you sure you want to enable the service '%1' at startup?\nThis requires administrator privileges.").arg(serviceName),
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::No) {
            return;
        }
        
        m_statusLabel->setText(QString("Enabling service %1...").arg(serviceName));
        setButtonsEnabled(false);
        
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                setButtonsEnabled(true);
                
                if (exitCode == 0) {
                    m_statusLabel->setText(QString("Service %1 enabled successfully").arg(serviceName));
                    fetchServiceDetails(row, serviceName);
                } else {
                    QString error = process->readAllStandardError();
                    m_statusLabel->setText(QString("Failed to enable service %1").arg(serviceName));
                    QMessageBox::critical(this, "Error", 
                        QString("Failed to enable service.\n%1").arg(error));
                }
                
                process->deleteLater();
            });
        
        process->start("pkexec", QStringList() << "systemctl" << "enable" << serviceName);
    }
    
    void disableService()
    {
        QList<QTableWidgetItem*> items = m_servicesTable->selectedItems();
        if (items.isEmpty()) {
            return;
        }
        
        int row = items.first()->row();
        QTableWidgetItem* nameItem = m_servicesTable->item(row, 0);
        if (!nameItem) {
            return;
        }
        
        QString serviceName = nameItem->text();
        
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            "Confirm Service Disable", 
            QString("Are you sure you want to disable the service '%1' at startup?\nThis requires administrator privileges.").arg(serviceName),
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::No) {
            return;
        }
        
        m_statusLabel->setText(QString("Disabling service %1...").arg(serviceName));
        setButtonsEnabled(false);
        
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                setButtonsEnabled(true);
                
                if (exitCode == 0) {
                    m_statusLabel->setText(QString("Service %1 disabled successfully").arg(serviceName));
                    fetchServiceDetails(row, serviceName);
                } else {
                    QString error = process->readAllStandardError();
                    m_statusLabel->setText(QString("Failed to disable service %1").arg(serviceName));
                    QMessageBox::critical(this, "Error", 
                        QString("Failed to disable service.\n%1").arg(error));
                }
                
                process->deleteLater();
            });
        
        process->start("pkexec", QStringList() << "systemctl" << "disable" << serviceName);
    }

private:
    void setButtonsEnabled(bool enabled)
    {
        m_refreshButton->setEnabled(enabled);
        m_startButton->setEnabled(enabled && !m_servicesTable->selectedItems().isEmpty());
        m_stopButton->setEnabled(enabled && !m_servicesTable->selectedItems().isEmpty());
        m_enableButton->setEnabled(enabled && !m_servicesTable->selectedItems().isEmpty());
        m_disableButton->setEnabled(enabled && !m_servicesTable->selectedItems().isEmpty());
        m_searchEdit->setEnabled(enabled);
    }

private:
    QTableWidget *m_servicesTable;
    QLabel *m_statusLabel;
    QPushButton *m_refreshButton;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
    QPushButton *m_enableButton;
    QPushButton *m_disableButton;
    QLineEdit *m_searchEdit;
    QStringList m_allServices;
};

class DiskUsageAnalyzerWidget : public QWidget
{
    Q_OBJECT

public:
    DiskUsageAnalyzerWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QLabel *infoLabel = new QLabel("This tab helps you analyze disk usage and find large files.", this);
        mainLayout->addWidget(infoLabel);
        
        QHBoxLayout *partitionsLayout = new QHBoxLayout();
        QLabel *partitionLabel = new QLabel("Select partition:", this);
        m_partitionsCombo = new QComboBox(this);
        connect(m_partitionsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
                this, &DiskUsageAnalyzerWidget::updatePartitionInfo);
        
        m_refreshPartitionsButton = new QPushButton("Refresh", this);
        connect(m_refreshPartitionsButton, &QPushButton::clicked, this, &DiskUsageAnalyzerWidget::refreshPartitions);
        
        partitionsLayout->addWidget(partitionLabel);
        partitionsLayout->addWidget(m_partitionsCombo);
        partitionsLayout->addWidget(m_refreshPartitionsButton);
        partitionsLayout->addStretch();
        
        mainLayout->addLayout(partitionsLayout);
        
        QGroupBox *partitionInfoBox = new QGroupBox("Partition Information", this);
        QGridLayout *infoLayout = new QGridLayout(partitionInfoBox);
        
        QLabel *sizeLabel = new QLabel("Total Size:", this);
        m_sizeValueLabel = new QLabel("-", this);
        QLabel *usedLabel = new QLabel("Used Space:", this);
        m_usedValueLabel = new QLabel("-", this);
        QLabel *freeLabel = new QLabel("Free Space:", this);
        m_freeValueLabel = new QLabel("-", this);
        QLabel *mountLabel = new QLabel("Mount Point:", this);
        m_mountValueLabel = new QLabel("-", this);
        
        infoLayout->addWidget(sizeLabel, 0, 0);
        infoLayout->addWidget(m_sizeValueLabel, 0, 1);
        infoLayout->addWidget(usedLabel, 0, 2);
        infoLayout->addWidget(m_usedValueLabel, 0, 3);
        infoLayout->addWidget(freeLabel, 1, 0);
        infoLayout->addWidget(m_freeValueLabel, 1, 1);
        infoLayout->addWidget(mountLabel, 1, 2);
        infoLayout->addWidget(m_mountValueLabel, 1, 3);
        
        mainLayout->addWidget(partitionInfoBox);
        
        QGroupBox *analysisBox = new QGroupBox("Directory Analysis", this);
        QVBoxLayout *analysisLayout = new QVBoxLayout(analysisBox);
        
        QHBoxLayout *dirLayout = new QHBoxLayout();
        QLabel *dirLabel = new QLabel("Directory:", this);
        m_directoryEdit = new QLineEdit(this);
        m_browseButton = new QPushButton("Browse", this);
        connect(m_browseButton, &QPushButton::clicked, this, &DiskUsageAnalyzerWidget::browseDirectory);
        
        dirLayout->addWidget(dirLabel);
        dirLayout->addWidget(m_directoryEdit);
        dirLayout->addWidget(m_browseButton);
        
        analysisLayout->addLayout(dirLayout);
        
        QHBoxLayout *filterLayout = new QHBoxLayout();
        QLabel *filterLabel = new QLabel("Filter:", this);
        m_filterEdit = new QLineEdit(this);
        m_filterEdit->setPlaceholderText("Enter extension (e.g., *.log) or pattern");
        
        QPushButton *filterButton = new QPushButton("Apply Filter", this);
        connect(filterButton, &QPushButton::clicked, this, &DiskUsageAnalyzerWidget::applyFilter);
        
        filterLayout->addWidget(filterLabel);
        filterLayout->addWidget(m_filterEdit);
        filterLayout->addWidget(filterButton);
        
        analysisLayout->addLayout(filterLayout);
        
        QHBoxLayout *largeFilesLayout = new QHBoxLayout();
        QLabel *sizeFilterLabel = new QLabel("Find files larger than:", this);
        m_sizeFilterSpinBox = new QSpinBox(this);
        m_sizeFilterSpinBox->setMinimum(1);
        m_sizeFilterSpinBox->setMaximum(10000);
        m_sizeFilterSpinBox->setValue(100);
        
        m_sizeUnitCombo = new QComboBox(this);
        m_sizeUnitCombo->addItems(QStringList() << "MB" << "GB");
        
        QPushButton *findLargeButton = new QPushButton("Find Large Files", this);
        connect(findLargeButton, &QPushButton::clicked, this, &DiskUsageAnalyzerWidget::findLargeFiles);
        
        largeFilesLayout->addWidget(sizeFilterLabel);
        largeFilesLayout->addWidget(m_sizeFilterSpinBox);
        largeFilesLayout->addWidget(m_sizeUnitCombo);
        largeFilesLayout->addWidget(findLargeButton);
        largeFilesLayout->addStretch();
        
        analysisLayout->addLayout(largeFilesLayout);
        
        QHBoxLayout *actionButtonLayout = new QHBoxLayout();
        
        QPushButton *analyzeButton = new QPushButton("Analyze Directory", this);
        connect(analyzeButton, &QPushButton::clicked, this, &DiskUsageAnalyzerWidget::analyzeDirectory);
        
        QPushButton *browseFinderButton = new QPushButton("Browse in File Browser", this);
        connect(browseFinderButton, &QPushButton::clicked, this, &DiskUsageAnalyzerWidget::browseFindDirectory);
        
        actionButtonLayout->addWidget(analyzeButton);
        actionButtonLayout->addWidget(browseFinderButton);
        
        analysisLayout->addLayout(actionButtonLayout);
        
        mainLayout->addWidget(analysisBox);
        
        QLabel *resultsLabel = new QLabel("Analysis Results:", this);
        mainLayout->addWidget(resultsLabel);
        
        m_resultsTable = new QTableWidget(0, 3, this);
        m_resultsTable->setHorizontalHeaderLabels(QStringList() << "Name" << "Size" << "Path");
        m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_resultsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_resultsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_resultsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        m_resultsTable->setMinimumHeight(200);
        connect(m_resultsTable, &QTableWidget::cellDoubleClicked, 
                this, &DiskUsageAnalyzerWidget::onTableItemDoubleClicked);
        
        mainLayout->addWidget(m_resultsTable);
        
        QHBoxLayout *paginationLayout = new QHBoxLayout();
        
        m_prevPageButton = new QPushButton("Previous Page", this);
        connect(m_prevPageButton, &QPushButton::clicked, this, [this]() {
            displayResultsPage(m_currentPage - 1);
        });
        m_prevPageButton->setEnabled(false);
        
        m_paginationLabel = new QLabel("Page 1 of 1", this);
        m_paginationLabel->setAlignment(Qt::AlignCenter);
        
        m_nextPageButton = new QPushButton("Next Page", this);
        connect(m_nextPageButton, &QPushButton::clicked, this, [this]() {
            displayResultsPage(m_currentPage + 1);
        });
        m_nextPageButton->setEnabled(false);
        
        paginationLayout->addWidget(m_prevPageButton);
        paginationLayout->addWidget(m_paginationLabel);
        paginationLayout->addWidget(m_nextPageButton);
        
        mainLayout->addLayout(paginationLayout);
        
        m_statusLabel = new QLabel("Ready", this);
        mainLayout->addWidget(m_statusLabel);
        
        refreshPartitions();
    }

public slots:
    void refreshPartitions()
    {
        m_statusLabel->setText("Refreshing partitions list...");
        m_refreshPartitionsButton->setEnabled(false);
        m_partitionsCombo->clear();
        
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                m_refreshPartitionsButton->setEnabled(true);
                
                if (exitCode == 0) {
                    QString output = process->readAllStandardOutput();
                    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
                    
                    for (const QString &line : lines) {
                        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                        if (parts.size() >= 6) {
                            QString device = parts[0];
                            QString mountpoint = parts[5];
                            
                            if (!device.startsWith("/dev/")) {
                                continue;
                            }
                            
                            m_partitionsCombo->addItem(QString("%1 (%2)").arg(device).arg(mountpoint), mountpoint);
                        }
                    }
                    
                    if (m_partitionsCombo->count() > 0) {
                        m_partitionsCombo->setCurrentIndex(0);
                        updatePartitionInfo(0);
                    }
                    
                    m_statusLabel->setText("Ready");
                } else {
                    m_statusLabel->setText("Failed to get partitions list");
                }
                
                process->deleteLater();
            });
        
        process->start("df", QStringList() << "-h");
    }

    void updatePartitionInfo(int index)
    {
        if (index < 0 || index >= m_partitionsCombo->count()) {
            return;
        }
        
        QString mountpoint = m_partitionsCombo->itemData(index).toString();
        m_statusLabel->setText(QString("Getting info for %1...").arg(mountpoint));
        
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode == 0) {
                    QString output = process->readAllStandardOutput();
                    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
                    
                    if (lines.size() > 1) {
                        QString dataLine = lines[1];
                        QStringList parts = dataLine.split(' ', Qt::SkipEmptyParts);
                        
                        if (parts.size() >= 6) {
                            m_sizeValueLabel->setText(parts[1]);
                            m_usedValueLabel->setText(parts[2] + " (" + parts[4] + ")");
                            m_freeValueLabel->setText(parts[3]);
                            m_mountValueLabel->setText(parts[5]);
                            
                            m_directoryEdit->setText(parts[5]);
                        }
                    }
                    
                    m_statusLabel->setText("Ready");
                } else {
                    m_statusLabel->setText("Failed to get partition info");
                }
                
                process->deleteLater();
            });
        
        process->start("df", QStringList() << "-h" << mountpoint);
    }
    
    void browseDirectory()
    {
        m_statusLabel->setText("Opening file browser for directory selection...");
        
        QString user = qgetenv("SUDO_USER");
        if (user.isEmpty()) {
            user = qgetenv("USER");
        }
        
        if (user.isEmpty()) {
            m_statusLabel->setText("Failed to determine user");
            return;
        }
        
        QString homeDir = "/home/" + user;
        
        QProcess *process = new QProcess(this);
        QStringList env = QProcess::systemEnvironment();
        env << "DISPLAY=" + qgetenv("DISPLAY");
        
        process->setEnvironment(env);
        
        process->start("bash", QStringList() 
            << "-c" 
            << QString("su %1 -c 'xdg-open %2' || echo 'Failed to open file browser'").arg(user).arg(homeDir));
        
        process->waitForFinished(5000);
        
        m_statusLabel->setText("Please select a directory in the opened file browser, then enter it manually in the Directory field.");
        process->deleteLater();
    }
    
    void browseFindDirectory()
    {
        QString directory = m_directoryEdit->text();
        if (directory.isEmpty()) {
            m_statusLabel->setText("No directory specified");
            return;
        }
        
        m_statusLabel->setText(QString("Opening %1 in file browser...").arg(directory));
        
        QString user = qgetenv("SUDO_USER");
        if (user.isEmpty()) {
            user = qgetenv("USER");
        }
        
        if (user.isEmpty()) {
            m_statusLabel->setText("Failed to determine user");
            return;
        }
        
        QProcess *process = new QProcess(this);
        QStringList env = QProcess::systemEnvironment();
        env << "DISPLAY=" + qgetenv("DISPLAY");
        
        process->setEnvironment(env);
        
        process->start("bash", QStringList() 
            << "-c" 
            << QString("su %1 -c 'xdg-open %2' || echo 'Failed to open directory'").arg(user).arg(directory));
        
        process->waitForFinished(5000);
        
        process->deleteLater();
        m_statusLabel->setText("File browser requested");
    }
    
    void analyzeDirectory()
    {
        QString directory = m_directoryEdit->text();
        if (directory.isEmpty()) {
            m_statusLabel->setText("No directory specified");
            return;
        }
        
        m_statusLabel->setText(QString("Analyzing directory %1...").arg(directory));
        m_resultsTable->clearContents();
        m_resultsTable->setRowCount(0);
        
        m_allResults.clear();
        m_currentPage = 0;
        m_prevPageButton->setEnabled(false);
        m_nextPageButton->setEnabled(false);
        m_paginationLabel->setText("Page 1 of 1");
        
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode == 0) {
                    QString output = process->readAllStandardOutput();
                    m_allResults = output.split('\n', Qt::SkipEmptyParts);
                    
                    int totalResults = m_allResults.size();
                    int totalPages = (totalResults + RESULTS_PER_PAGE - 1) / RESULTS_PER_PAGE;
                    
                    displayResultsPage(0);
                    
                    if (totalResults > RESULTS_PER_PAGE) {
                        m_statusLabel->setText(QString("Analysis complete. Found %1 results. Showing page 1 of %2.")
                            .arg(totalResults)
                            .arg(totalPages));
                    } else {
                        m_statusLabel->setText(QString("Analysis complete. Found %1 results.")
                            .arg(totalResults));
                    }
                } else {
                    m_statusLabel->setText("Failed to analyze directory");
                }
                
                process->deleteLater();
            });
        
        process->start("bash", QStringList() << "-c" << 
            QString("find \"%1\" -type f -not -path \"*/\\.*\" | sort -k2 | "
                   "while read file; do "
                   "  echo \"$(basename \"$file\")|$(du -h \"$file\" | cut -f1)|$file\"; "
                   "done | sort -hr -k2").arg(directory));
    }
    
    void onTableItemDoubleClicked(int row, int column)
    {
        QTableWidgetItem *pathItem = m_resultsTable->item(row, 2);
        if (pathItem) {
            QString filePath = pathItem->text();
            QString dirPath = QFileInfo(filePath).absolutePath();
            
            QProcess *process = new QProcess(this);
            process->start("xdg-open", QStringList() << dirPath);
            
            m_statusLabel->setText(QString("Opening location: %1").arg(dirPath));
        }
    }
    
    void applyFilter()
    {
        QString directory = m_directoryEdit->text();
        QString filter = m_filterEdit->text();
        
        if (directory.isEmpty()) {
            m_statusLabel->setText("No directory specified");
            return;
        }
        
        if (filter.isEmpty()) {
            m_statusLabel->setText("No filter specified");
            return;
        }
        
        m_statusLabel->setText(QString("Filtering files in %1 with pattern %2...").arg(directory).arg(filter));
        m_resultsTable->clearContents();
        m_resultsTable->setRowCount(0);
        
        m_allResults.clear();
        m_currentPage = 0;
        m_prevPageButton->setEnabled(false);
        m_nextPageButton->setEnabled(false);
        m_paginationLabel->setText("Page 1 of 1");
        
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode == 0) {
                    QString output = process->readAllStandardOutput();
                    m_allResults = output.split('\n', Qt::SkipEmptyParts);
                    
                    int totalResults = m_allResults.size();
                    int totalPages = (totalResults + RESULTS_PER_PAGE - 1) / RESULTS_PER_PAGE;
                    
                    displayResultsPage(0);
                    
                    if (totalResults > RESULTS_PER_PAGE) {
                        m_statusLabel->setText(QString("Filter applied. Found %1 results. Showing page 1 of %2.")
                            .arg(totalResults)
                            .arg(totalPages));
                    } else {
                        m_statusLabel->setText(QString("Filter applied. Found %1 results.")
                            .arg(totalResults));
                    }
                } else {
                    m_statusLabel->setText("Failed to apply filter");
                }
                
                process->deleteLater();
            });
        
        QString findCommand;
        if (filter.startsWith("*.")) {
            QString ext = filter.mid(1);
            findCommand = QString("find \"%1\" -type f -name \"%2\" | "
                                  "while read file; do "
                                  "  echo \"$(basename \"$file\")|$(du -h \"$file\" | cut -f1)|$file\"; "
                                  "done | sort -hr -k2").arg(directory).arg(filter);
        } else {
            findCommand = QString("find \"%1\" -type f -name \"*%2*\" | "
                                  "while read file; do "
                                  "  echo \"$(basename \"$file\")|$(du -h \"$file\" | cut -f1)|$file\"; "
                                  "done | sort -hr -k2").arg(directory).arg(filter);
        }
        
        process->start("bash", QStringList() << "-c" << findCommand);
    }
    
    void findLargeFiles()
    {
        QString directory = m_directoryEdit->text();
        int sizeThreshold = m_sizeFilterSpinBox->value();
        QString unit = m_sizeUnitCombo->currentText();
        
        if (directory.isEmpty()) {
            m_statusLabel->setText("No directory specified");
            return;
        }
        
        qint64 thresholdBytes;
        if (unit == "MB") {
            thresholdBytes = static_cast<qint64>(sizeThreshold) * 1024 * 1024;
        } else {
            thresholdBytes = static_cast<qint64>(sizeThreshold) * 1024 * 1024 * 1024;
        }
        
        m_statusLabel->setText(QString("Finding files larger than %1 %2 in %3...").arg(sizeThreshold).arg(unit).arg(directory));
        m_resultsTable->clearContents();
        m_resultsTable->setRowCount(0);
        
        m_allResults.clear();
        m_currentPage = 0;
        m_prevPageButton->setEnabled(false);
        m_nextPageButton->setEnabled(false);
        m_paginationLabel->setText("Page 1 of 1");
        
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode == 0) {
                    QString output = process->readAllStandardOutput();
                    m_allResults = output.split('\n', Qt::SkipEmptyParts);
                    
                    int totalResults = m_allResults.size();
                    int totalPages = (totalResults + RESULTS_PER_PAGE - 1) / RESULTS_PER_PAGE;
                    
                    displayResultsPage(0);
                    
                    if (totalResults > RESULTS_PER_PAGE) {
                        m_statusLabel->setText(QString("Found %1 large files. Showing page 1 of %2.")
                            .arg(totalResults)
                            .arg(totalPages));
                    } else {
                        m_statusLabel->setText(QString("Found %1 large files.")
                            .arg(totalResults));
                    }
                } else {
                    m_statusLabel->setText("Failed to find large files");
                }
                
                process->deleteLater();
            });
        
        QString findCommand = QString("find \"%1\" -type f -size +%2c | "
                                     "while read file; do "
                                     "  echo \"$(basename \"$file\")|$(du -h \"$file\" | cut -f1)|$file\"; "
                                     "done | sort -hr -k2").arg(directory).arg(thresholdBytes);
        
        process->start("bash", QStringList() << "-c" << findCommand);
    }

    void displayResultsPage(int page)
    {
        if (m_allResults.isEmpty()) {
            return;
        }

        int totalPages = (m_allResults.size() + RESULTS_PER_PAGE - 1) / RESULTS_PER_PAGE;
        m_currentPage = qBound(0, page, totalPages - 1);

        updatePaginationControls(totalPages);

        int startIdx = m_currentPage * RESULTS_PER_PAGE;
        int endIdx = qMin(startIdx + RESULTS_PER_PAGE, m_allResults.size());
        int itemCount = endIdx - startIdx;

        m_resultsTable->clearContents();
        m_resultsTable->setRowCount(itemCount);

        int row = 0;
        for (int i = startIdx; i < endIdx; i++) {
            const QString &line = m_allResults[i];
            QStringList parts = line.split('|');
            if (parts.size() >= 3) {
                QTableWidgetItem *nameItem = new QTableWidgetItem(parts[0].trimmed());
                QTableWidgetItem *sizeItem = new QTableWidgetItem(parts[1].trimmed());
                QTableWidgetItem *pathItem = new QTableWidgetItem(parts[2].trimmed());
                
                m_resultsTable->setItem(row, 0, nameItem);
                m_resultsTable->setItem(row, 1, sizeItem);
                m_resultsTable->setItem(row, 2, pathItem);
                row++;
            }
        }
    }

    void updatePaginationControls(int totalPages)
    {
        m_prevPageButton->setEnabled(m_currentPage > 0);
        m_nextPageButton->setEnabled(m_currentPage < totalPages - 1);
        
        m_paginationLabel->setText(QString("Page %1 of %2")
            .arg(m_currentPage + 1)
            .arg(totalPages));
    }

private:
    QComboBox *m_partitionsCombo;
    QPushButton *m_refreshPartitionsButton;
    QLabel *m_sizeValueLabel;
    QLabel *m_usedValueLabel;
    QLabel *m_freeValueLabel;
    QLabel *m_mountValueLabel;
    QLineEdit *m_directoryEdit;
    QPushButton *m_browseButton;
    QLineEdit *m_filterEdit;
    QSpinBox *m_sizeFilterSpinBox;
    QComboBox *m_sizeUnitCombo;
    QTableWidget *m_resultsTable;
    QLabel *m_statusLabel;
    
    QStringList m_allResults;
    int m_currentPage = 0;
    static const int RESULTS_PER_PAGE = 200;
    QPushButton *m_prevPageButton;
    QPushButton *m_nextPageButton;
    QLabel *m_paginationLabel;
};

class PacmanCacheCleaner : public QMainWindow
{
    Q_OBJECT

public:
    PacmanCacheCleaner(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle("Pacman Cache Cleaner (Administrator)");
        setMinimumSize(750, 550);

        m_tabWidget = new QTabWidget(this);
        setCentralWidget(m_tabWidget);
        
        m_cacheTab = new CacheManagementWidget(this);
        m_orphanedTab = new OrphanedPackagesWidget(this);
        m_logsTab = new SystemLogsWidget(this);
        m_servicesTab = new SystemServicesWidget(this);
        m_diskUsageTab = new DiskUsageAnalyzerWidget(this);
        
        m_tabWidget->addTab(m_cacheTab, "Cache Management");
        m_tabWidget->addTab(m_orphanedTab, "Orphaned Packages");
        m_tabWidget->addTab(m_logsTab, "System Logs");
        m_tabWidget->addTab(m_servicesTab, "System Services");
        m_tabWidget->addTab(m_diskUsageTab, "Disk Usage");
        
        m_statusLabel = new QLabel("Ready", this);
        statusBar()->addWidget(m_statusLabel);
        
        connect(m_tabWidget, &QTabWidget::currentChanged, this, &PacmanCacheCleaner::onTabChanged);
    }
    
private slots:
    void onTabChanged(int index)
    {
        if (index == 1) {
            m_orphanedTab->listOrphanedPackages();
        }
    }

private:
    QTabWidget *m_tabWidget;
    CacheManagementWidget *m_cacheTab;
    OrphanedPackagesWidget *m_orphanedTab;
    SystemLogsWidget *m_logsTab;
    SystemServicesWidget *m_servicesTab;
    DiskUsageAnalyzerWidget *m_diskUsageTab;
    QLabel *m_statusLabel;
};

#include "main.moc"

bool isRoot()
{
    return getuid() == 0;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Pacman Cache Cleaner");
    app.setApplicationDisplayName("Pacman Cache Cleaner");
    
    if (!isRoot()) {
        QMessageBox::information(nullptr, "Administrator Privileges Required",
            "This application requires administrator privileges to function properly.\n"
            "The application will now restart with elevated privileges.");
        
        QString execPath = QCoreApplication::applicationFilePath();
        
        QProcess process;
        process.startDetached("pkexec", QStringList() << execPath);
        
        return 0;
    }
    
    PacmanCacheCleaner mainWindow;
    mainWindow.show();
    
    return app.exec();
}