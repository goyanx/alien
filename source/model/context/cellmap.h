#ifndef CELLMAP_H
#define CELLMAP_H

#include "model/Definitions.h"

class CellMap
	: public QObject
{
	Q_OBJECT
public:
	CellMap(QObject* parent = nullptr) : QObject(parent) {}
	virtual ~CellMap() {}

	virtual void init(SpaceMetric* topo, MapCompartment* compartment) = 0;
	virtual void clear() = 0;

	virtual void setCell(QVector3D pos, Cell* cell) = 0;
	virtual void removeCellIfPresent(QVector3D pos, Cell* cell) = 0;
	virtual Cell* getCell(QVector3D pos) const = 0;
	inline Cell* getCellFast(IntVector2D const& intPos) const;

	//advanced functions
	virtual CellClusterSet getNearbyClusters(QVector3D const& pos, qreal r) const = 0;
	virtual CellCluster* getNearbyClusterFast(QVector3D const& pos, qreal r, qreal minMass, qreal maxMass, CellCluster* exclude) const = 0;
	using CellSelectFunction = bool(*)(Cell*);
	virtual QList< Cell* > getNearbySpecificCells(QVector3D const& pos, qreal r, CellSelectFunction selection) const = 0;

	virtual void serializePrimitives(QDataStream& stream) const = 0;
	virtual void deserializePrimitives(QDataStream& stream, QMap< quint64, Cell* > const& oldIdCellMap) = 0;

protected:
	Cell*** _cellGrid = nullptr;
};

Cell * CellMap::getCellFast(IntVector2D const& intPos) const
{
	return _cellGrid[intPos.x][intPos.y];
}


#endif //CELLMAP_H