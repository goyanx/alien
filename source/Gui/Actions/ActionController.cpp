﻿#include <QFileDialog>
#include <QMessageBox>
#include <QAction>

#include "Model/Api/SimulationController.h"
#include "Model/Api/Serializer.h"
#include "Model/Api/SymbolTable.h"

#include "Gui/Toolbar/ToolbarController.h"
#include "Gui/Toolbar/ToolbarContext.h"
#include "Gui/VisualEditor/VisualEditController.h"
#include "Gui/DataEditor/DataEditController.h"
#include "Gui/DataEditor/DataEditContext.h"
#include "Gui/Dialogs/NewSimulationDialog.h"
#include "Gui/Dialogs/SimulationParametersDialog.h"
#include "Gui/Dialogs/SymbolTableDialog.h"
#include "Gui/Settings.h"
#include "Gui/SerializationHelper.h"
#include "Gui/InfoController.h"
#include "Gui/MainController.h"
#include "Gui/MainModel.h"
#include "Gui/MainView.h"
#include "Gui/Notifier.h"

#include "ActionModel.h"
#include "ActionController.h"
#include "ActionHolder.h"

ActionController::ActionController(QObject * parent)
	: QObject(parent)
{
	_model = new ActionModel(this);
}

void ActionController::init(MainController * mainController, MainModel* mainModel, MainView* mainView, VisualEditController* visualEditor
	, Serializer* serializer, InfoController* infoController, DataEditController* dataEditor, ToolbarController* toolbar
	, DataRepository* repository, Notifier* notifier)
{
	_mainController = mainController;
	_mainModel = mainModel;
	_mainView = mainView;
	_visualEditor = visualEditor;
	_serializer = serializer;
	_infoController = infoController;
	_dataEditor = dataEditor;
	_toolbar = toolbar;
	_repository = repository;
	_notifier = notifier;

	connect(_notifier, &Notifier::notify, this, &ActionController::receivedNotifications);

	auto actions = _model->getActionHolder();
	connect(actions->actionNewSimulation, &QAction::triggered, this, &ActionController::onNewSimulation);
	connect(actions->actionSaveSimulation, &QAction::triggered, this, &ActionController::onSaveSimulation);
	connect(actions->actionLoadSimulation, &QAction::triggered, this, &ActionController::onLoadSimulation);
	connect(actions->actionRunSimulation, &QAction::toggled, this, &ActionController::onRunClicked);
	connect(actions->actionRunStepForward, &QAction::triggered, this, &ActionController::onStepForward);
	connect(actions->actionRunStepBackward, &QAction::triggered, this, &ActionController::onStepBackward);
	connect(actions->actionSnapshot, &QAction::triggered, this, &ActionController::onMakeSnapshot);
	connect(actions->actionRestore, &QAction::triggered, this, &ActionController::onRestoreSnapshot);
	connect(actions->actionExit, &QAction::triggered, _mainView, &MainView::close);
	connect(actions->actionZoomIn, &QAction::triggered, this, &ActionController::onZoomInClicked);
	connect(actions->actionZoomOut, &QAction::triggered, this, &ActionController::onZoomOutClicked);
	connect(actions->actionEditor, &QAction::toggled, this, &ActionController::onSetEditorMode);
	connect(actions->actionEditSimParameters, &QAction::triggered, this, &ActionController::onEditSimulationParameters);
	connect(actions->actionLoadSimParameters, &QAction::triggered, this, &ActionController::onLoadSimulationParameters);
	connect(actions->actionSaveSimParameters, &QAction::triggered, this, &ActionController::onSaveSimulationParameters);
	connect(actions->actionEditSymbols, &QAction::triggered, this, &ActionController::onEditSymbolTable);
	connect(actions->actionLoadSymbols, &QAction::triggered, this, &ActionController::onLoadSymbolTable);
	connect(actions->actionSaveSymbols, &QAction::triggered, this, &ActionController::onSaveSymbolTable);

	connect(actions->actionNewCell, &QAction::triggered, this, &ActionController::onNewCell);
	connect(actions->actionNewParticle, &QAction::triggered, this, &ActionController::onNewParticle);
	connect(actions->actionLoadCol, &QAction::triggered, this, &ActionController::onLoadCollection);
	connect(actions->actionSaveCol, &QAction::triggered, this, &ActionController::onSaveCollection);
	connect(actions->actionCopyCol, &QAction::triggered, this, &ActionController::onCopyCollection);
	connect(actions->actionPasteCol, &QAction::triggered, this, &ActionController::onPasteCollection);
	connect(actions->actionDeleteSel, &QAction::triggered, this, &ActionController::onDeleteSelection);
	connect(actions->actionDeleteCol, &QAction::triggered, this, &ActionController::onDeleteCollection);
	connect(actions->actionNewToken, &QAction::triggered, this, &ActionController::onNewToken);
	connect(actions->actionDeleteToken, &QAction::triggered, this, &ActionController::onDeleteToken);
	connect(actions->actionShowCellInfo, &QAction::toggled, this, &ActionController::onToggleCellInfo);
}

ActionHolder * ActionController::getActionHolder()
{
	return _model->getActionHolder();
}

void ActionController::onRunClicked(bool run)
{
	auto actions = _model->getActionHolder();
	if (run) {
		actions->actionRunSimulation->setIcon(QIcon("://Icons/pause.png"));
		actions->actionRunStepForward->setEnabled(false);
	}
	else {
		actions->actionRunSimulation->setIcon(QIcon("://Icons/play.png"));
		actions->actionRunStepForward->setEnabled(true);
	}
	actions->actionRunStepBackward->setEnabled(false);

	_mainController->onRunSimulation(run);
}

void ActionController::onStepForward()
{
	_mainController->onStepForward();
	_model->getActionHolder()->actionRunStepBackward->setEnabled(true);
}

void ActionController::onStepBackward()
{
	bool emptyStack = false;
	_mainController->onStepBackward(emptyStack);
	if (emptyStack) {
		_model->getActionHolder()->actionRunStepBackward->setEnabled(false);
	}
	_visualEditor->refresh();
}

void ActionController::onMakeSnapshot()
{
	_mainController->onMakeSnapshot();
	_model->getActionHolder()->actionRestore->setEnabled(true);
}

void ActionController::onRestoreSnapshot()
{
	_mainController->onRestoreSnapshot();
	_visualEditor->refresh();
}

void ActionController::onZoomInClicked()
{
	_visualEditor->zoom(2.0);
	updateZoomFactor();
}

void ActionController::onZoomOutClicked()
{
	_visualEditor->zoom(0.5);
	updateZoomFactor();
}

void ActionController::onSetEditorMode(bool editMode)
{
	_model->setEditMode(editMode);
	if (editMode) {
		_visualEditor->setActiveScene(ActiveScene::ItemScene);
	}
	else {
		_visualEditor->setActiveScene(ActiveScene::PixelScene);
	}
	updateActionsEnableState();

	Q_EMIT _toolbar->getContext()->show(editMode);
	Q_EMIT _dataEditor->getContext()->show(editMode);
}

void ActionController::onNewSimulation()
{
	NewSimulationDialog dialog(_mainModel->getSimulationParameters(), _mainModel->getSymbolTable(), _serializer, _mainView);
	if (dialog.exec()) {
		NewSimulationConfig config{
			dialog.getMaxThreads(), dialog.getGridSize(), dialog.getUniverseSize(), dialog.getSymbolTable(), dialog.getSimulationParameters(), dialog.getEnergy()
		};
		_mainController->onNewSimulation(config);
		updateZoomFactor();
		_model->getActionHolder()->actionRunSimulation->setChecked(false);
		_model->getActionHolder()->actionRestore->setEnabled(false);
		_model->getActionHolder()->actionRunStepBackward->setEnabled(false);
		onRunClicked(false);
	}
}

void ActionController::onSaveSimulation()
{
	QString filename = QFileDialog::getSaveFileName(_mainView, "Save Simulation", "", "Alien Simulation(*.sim)");
	if (!filename.isEmpty()) {
		_mainController->onSaveSimulation(filename.toStdString());
	}
}

void ActionController::onLoadSimulation()
{
	QString filename = QFileDialog::getOpenFileName(_mainView, "Load Simulation", "", "Alien Simulation (*.sim)");
	if (!filename.isEmpty()) {
		if (_mainController->onLoadSimulation(filename.toStdString())) {
			updateZoomFactor();
			_model->getActionHolder()->actionRunSimulation->setChecked(false);
			_model->getActionHolder()->actionRestore->setEnabled(false);
			_model->getActionHolder()->actionRunStepBackward->setEnabled(false);
			onRunClicked(false);
		}
		else {
			QMessageBox msgBox(QMessageBox::Critical, "Error", "An error occurred. Specified simulation could not loaded.");
			msgBox.exec();
		}
	}
}

void ActionController::onEditSimulationParameters()
{
	SimulationParametersDialog dialog(_mainModel->getSimulationParameters()->clone(), _serializer, _mainView);
	if (dialog.exec()) {
		_mainModel->setSimulationParameters(dialog.getSimulationParameters());
		_mainController->onUpdateSimulationParametersForRunningSimulation();
	}
}

void ActionController::onLoadSimulationParameters()
{
	QString filename = QFileDialog::getOpenFileName(_mainView, "Load Simulation Parameters", "", "Alien Simulation Parameters(*.par)");
	if (!filename.isEmpty()) {
		SimulationParameters* parameters;
		if (SerializationHelper::loadFromFile<SimulationParameters*>(filename.toStdString(), [&](string const& data) { return _serializer->deserializeSimulationParameters(data); }, parameters)) {
			_mainModel->setSimulationParameters(parameters);
			_mainController->onUpdateSimulationParametersForRunningSimulation();
		}
		else {
			QMessageBox msgBox(QMessageBox::Critical, "Error", "An error occurred. Specified simulation parameter file could not loaded.");
			msgBox.exec();
		}
	}
}

void ActionController::onSaveSimulationParameters()
{
	QString filename = QFileDialog::getSaveFileName(_mainView, "Save Simulation Parameters", "", "Alien Simulation Parameters(*.par)");
	if (!filename.isEmpty()) {
		if (!SerializationHelper::saveToFile(filename.toStdString(), [&]() { return _serializer->serializeSimulationParameters(_mainModel->getSimulationParameters()); })) {
			QMessageBox msgBox(QMessageBox::Critical, "Error", "An error occurred. Simulation parameters could not saved.");
			msgBox.exec();
		}
	}

}

void ActionController::onEditSymbolTable()
{
	auto origSymbols = _mainModel->getSymbolTable();
	SymbolTableDialog dialog(origSymbols->clone(), _serializer, _mainView);
	if (dialog.exec()) {
		origSymbols->getSymbolsFrom(dialog.getSymbolTable());
		Q_EMIT _dataEditor->getContext()->refresh();
	}
}

void ActionController::onLoadSymbolTable()
{
	QString filename = QFileDialog::getOpenFileName(_mainView, "Load Symbol Table", "", "Alien Symbol Table(*.sym)");
	if (!filename.isEmpty()) {
		SymbolTable* symbolTable;
		if (SerializationHelper::loadFromFile<SymbolTable*>(filename.toStdString(), [&](string const& data) { return _serializer->deserializeSymbolTable(data); }, symbolTable)) {
			_mainModel->getSymbolTable()->getSymbolsFrom(symbolTable);
			delete symbolTable;
			Q_EMIT _dataEditor->getContext()->refresh();
		}
		else {
			QMessageBox msgBox(QMessageBox::Critical, "Error", "An error occurred. Specified symbol table could not loaded.");
			msgBox.exec();
		}
	}
}

void ActionController::onSaveSymbolTable()
{
	QString filename = QFileDialog::getSaveFileName(_mainView, "Save Symbol Table", "", "Alien Symbol Table (*.sym)");
	if (!filename.isEmpty()) {
		if (!SerializationHelper::saveToFile(filename.toStdString(), [&]() { return _serializer->serializeSymbolTable(_mainModel->getSymbolTable()); })) {
			QMessageBox msgBox(QMessageBox::Critical, "Error", "An error occurred. Symbol table could not saved.");
			msgBox.exec();
			return;
		}
	}
}

void ActionController::onNewCell()
{
	_repository->addAndSelectCell(_model->getPositionDeltaForNewEntity());
	_repository->reconnectSelectedCells();
	Q_EMIT _notifier->notify({
		Receiver::DataEditor,
		Receiver::Simulation,
		Receiver::VisualEditor,
		Receiver::ActionController
	}, UpdateDescription::All);
}

void ActionController::onNewParticle()
{
	_repository->addAndSelectParticle(_model->getPositionDeltaForNewEntity());
	Q_EMIT _notifier->notify({
		Receiver::DataEditor,
		Receiver::Simulation,
		Receiver::VisualEditor,
		Receiver::ActionController
	}, UpdateDescription::All);
}

void ActionController::onLoadCollection()
{
	QString filename = QFileDialog::getOpenFileName(_mainView, "Load Collection", "", "Alien Collection (*.aco)");
	if (!filename.isEmpty()) {
		DataDescription desc;
		if (SerializationHelper::loadFromFile<DataDescription>(filename.toStdString(), [&](string const& data) { return _serializer->deserializeDataDescription(data); }, desc)) {
			_repository->addAndSelectData(desc, _model->getPositionDeltaForNewEntity());
			Q_EMIT _notifier->notify({
				Receiver::DataEditor,
				Receiver::Simulation,
				Receiver::VisualEditor,
				Receiver::ActionController
			}, UpdateDescription::All);
		}
		else {
			QMessageBox msgBox(QMessageBox::Critical, "Error", "An error occurred. Specified collection could not loaded.");
			msgBox.exec();
		}
	}
}

void ActionController::onSaveCollection()
{
	QString filename = QFileDialog::getSaveFileName(_mainView, "Save Collection", "", "Alien Collection (*.aco)");
	if (!filename.isEmpty()) {
		if (!SerializationHelper::saveToFile(filename.toStdString(), [&]() { return _serializer->serializeDataDescription(_repository->getExtendedSelection()); })) {
			QMessageBox msgBox(QMessageBox::Critical, "Error", "An error occurred. Collection could not saved.");
			msgBox.exec();
			return;
		}
	}
}

void ActionController::onCopyCollection()
{
	DataDescription copiedData = _repository->getExtendedSelection();
	_model->setCopiedData(copiedData);
	_model->setCollectionCopied(true);
	updateActionsEnableState();
}

void ActionController::onPasteCollection()
{
	DataDescription copiedData = _model->getCopiedData();
	_repository->addAndSelectData(copiedData, _model->getPositionDeltaForNewEntity());
	Q_EMIT _notifier->notify({
		Receiver::DataEditor,
		Receiver::Simulation,
		Receiver::VisualEditor,
		Receiver::ActionController
	}, UpdateDescription::All);
}

void ActionController::onDeleteSelection()
{
	_repository->deleteSelection();
	Q_EMIT _notifier->notify({
		Receiver::DataEditor,
		Receiver::Simulation,
		Receiver::VisualEditor,
		Receiver::ActionController
	}, UpdateDescription::All);
}

void ActionController::onDeleteCollection()
{
	_repository->deleteExtendedSelection();
	Q_EMIT _notifier->notify({
		Receiver::DataEditor,
		Receiver::Simulation,
		Receiver::VisualEditor,
		Receiver::ActionController
	}, UpdateDescription::All);
}

void ActionController::onNewToken()
{
	_repository->addToken();
	Q_EMIT _notifier->notify({
		Receiver::DataEditor,
		Receiver::Simulation,
		Receiver::VisualEditor,
		Receiver::ActionController
	}, UpdateDescription::All);
}

void ActionController::onDeleteToken()
{
	_repository->deleteToken();
	Q_EMIT _notifier->notify({
		Receiver::DataEditor,
		Receiver::Simulation,
		Receiver::VisualEditor,
		Receiver::ActionController
	}, UpdateDescription::All);
}

void ActionController::onToggleCellInfo(bool showInfo)
{
	Q_EMIT _notifier->toggleCellInfo(showInfo);
}


void ActionController::receivedNotifications(set<Receiver> const & targets)
{
	if (targets.find(Receiver::ActionController) == targets.end()) {
		return;
	}

	int selectedCells = _repository->getSelectedCellIds().size();
	int selectedParticles = _repository->getSelectedParticleIds().size();
	int tokenOfSelectedCell = 0;
	int freeTokenOfSelectedCell = 0;

	if (selectedCells == 1 && selectedParticles == 0) {
		uint64_t selectedCellId = *_repository->getSelectedCellIds().begin();
		if (auto tokens = _repository->getCellDescRef(selectedCellId).tokens) {
			tokenOfSelectedCell = tokens->size();
			freeTokenOfSelectedCell = _mainModel->getSimulationParameters()->cellMaxToken - tokenOfSelectedCell;
		}
	}

	_model->setEntitySelected(selectedCells == 1 || selectedParticles == 1);
	_model->setCellWithTokenSelected(tokenOfSelectedCell > 0);
	_model->setCellWithFreeTokenSelected(freeTokenOfSelectedCell > 0);
	_model->setCollectionSelected(selectedCells > 0 || selectedParticles > 0);

	updateActionsEnableState();
}

void ActionController::updateZoomFactor()
{
	_infoController->setZoomFactor(_visualEditor->getZoomFactor());
}

void ActionController::updateActionsEnableState()
{
	bool visible = _model->isEditMode();
	bool entitySelected = _model->isEntitySelected();
	bool entityCopied = _model->isEntityCopied();
	bool cellWithTokenSelected = _model->isCellWithTokenSelected();
	bool cellWithFreeTokenSelected = _model->isCellWithFreeTokenSelected();
	bool tokenCopied = _model->isTokenCopied();
	bool collectionSelected = _model->isCollectionSelected();
	bool collectionCopied = _model->isCollectionCopied();

	auto actions = _model->getActionHolder();
	actions->actionShowCellInfo->setEnabled(visible);

	actions->actionNewCell->setEnabled(visible);
	actions->actionNewParticle->setEnabled(visible);
	actions->actionCopyEntity->setEnabled(visible && entitySelected);
	actions->actionPasteEntity->setEnabled(visible && entityCopied);
	actions->actionDeleteEntity->setEnabled(visible && entitySelected);
	actions->actionNewToken->setEnabled(visible && entitySelected);
	actions->actionCopyToken->setEnabled(visible && entitySelected);
	actions->actionPasteToken->setEnabled(visible && cellWithFreeTokenSelected && tokenCopied);
	actions->actionDeleteToken->setEnabled(visible && cellWithTokenSelected);

	actions->actionNewRectangle->setEnabled(visible);
	actions->actionNewHexagon->setEnabled(visible);
	actions->actionNewParticles->setEnabled(visible);
	actions->actionLoadCol->setEnabled(visible);
	actions->actionSaveCol->setEnabled(visible && collectionSelected);
	actions->actionCopyCol->setEnabled(visible && collectionSelected);
	actions->actionPasteCol->setEnabled(visible && collectionCopied);
	actions->actionDeleteSel->setEnabled(visible && collectionSelected);
	actions->actionDeleteCol->setEnabled(visible && collectionSelected);
	actions->actionMultiplyRandom->setEnabled(visible && collectionSelected);
	actions->actionMultiplyArrangement->setEnabled(visible && collectionSelected);
}
