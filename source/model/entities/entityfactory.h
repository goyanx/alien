#ifndef ENTITYFACTORY_H
#define ENTITYFACTORY_H

#include "model/Definitions.h"
#include "Descriptions.h"
#include "LightDescriptions.h"

class EntityFactory
{
public:
    virtual ~EntityFactory () {}

	virtual CellCluster* build(CellClusterDescription const& desc, UnitContext* context) const = 0;
	virtual Cell* build(CellDescription const& desc, UnitContext* context) const = 0;
	virtual Token* build(TokenDescription const& desc, UnitContext* context) const = 0;
	virtual EnergyParticle* build(EnergyParticleDescription const& desc, UnitContext* context) const = 0;

	virtual CellCluster* build(CellClusterLightDescription const& desc, UnitContext* context) const = 0;
	virtual EnergyParticle* build(EnergyParticleLightDescription const& desc, UnitContext* context) const = 0;
};

#endif // ENTITYFACTORY_H
