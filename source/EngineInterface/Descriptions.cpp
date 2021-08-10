#include <QMatrix4x4>

#include "Descriptions.h"
#include "ChangeDescriptions.h"
#include "Physics.h"


bool TokenDescription::operator==(TokenDescription const& other) const {
	return energy == other.energy
		&& data == other.data;
}

namespace
{
    boost::optional<list<ConnectionDescription>> convert(
        boost::optional<std::list<ConnectionChangeDescription>> const& connections)
    {
        if (!connections) {
            return boost::none;
        }
        std::list<ConnectionDescription> result;
        for (auto const& connectionChange : *connections) {
            ConnectionDescription connection;
            connection.cellId = connectionChange.cellId;
            connection.distance = connectionChange.distance;
            connection.angleFromPrevious = connectionChange.angleFromPrevious;
            result.emplace_back(connection);
        }
        return result;
    }

}

CellDescription::CellDescription(CellChangeDescription const & change)
{
	id = change.id;
	pos = static_cast<boost::optional<QVector2D>>(change.pos);
	energy = static_cast<boost::optional<double>>(change.energy);
	maxConnections = static_cast<boost::optional<int>>(change.maxConnections);
    connections =
        convert(static_cast<boost::optional<std::list<ConnectionChangeDescription>>>(change.connectingCells));
	tokenBlocked = change.tokenBlocked.getOptionalValue();	//static_cast<boost::optional<bool>> doesn't work for some reason
	tokenBranchNumber = static_cast<boost::optional<int>>(change.tokenBranchNumber);
	metadata = static_cast<boost::optional<CellMetadata>>(change.metadata);
	cellFeature = static_cast<boost::optional<CellFeatureDescription>>(change.cellFeatures);
	tokens = static_cast<boost::optional<vector<TokenDescription>>>(change.tokens);
    tokenUsages = static_cast<boost::optional<int>>(change.tokenUsages);
}

CellDescription& CellDescription::addToken(TokenDescription const& value)
{
	if (!tokens) {
		tokens = vector<TokenDescription>();
	}
	tokens->push_back(value);
	return *this;
}

CellDescription& CellDescription::addToken(uint index, TokenDescription const& value)
{
	if (!tokens) {
		tokens = vector<TokenDescription>();
	}
	tokens->insert(tokens->begin() + index, value);
	return *this;
}

CellDescription& CellDescription::delToken(uint index)
{
	CHECK(tokens);
	tokens->erase(tokens->begin() + index);
	return *this;
}

QVector2D CellDescription::getPosRelativeTo(ClusterDescription const & cluster) const
{
	QMatrix4x4 transform;
	transform.setToIdentity();
	transform.translate(*cluster.pos);
	transform.rotate(*cluster.angle, 0.0, 0.0, 1.0);
	transform = transform.inverted();
	return transform.map(QVector3D(*pos)).toVector2D();
}

bool CellDescription::isConnectedTo(uint64_t id) const
{
    return std::find_if(
               connections->begin(),
               connections->end(),
               [&id](auto const& connection) { return connection.cellId == id; })
        != connections->end();
}

ClusterDescription& ClusterDescription::addConnection(
    uint64_t const& cellId1,
    uint64_t const& cellId2,
    std::unordered_map<uint64_t, int>& cache)
{
    auto& cell1 = getCellRef(cellId1, cache);
    auto& cell2 = getCellRef(cellId2, cache);

	auto addConnection = [this, &cache](auto& cell, auto& otherCell) {
        if (!cell.connections) {
            cell.connections = list<ConnectionDescription>();
        }
        CHECK(cell.connections->size() < *cell.maxConnections);

        auto newAngle = Physics::angleOfVector(*otherCell.pos - *cell.pos);
        if (cell.id == 281474976710658 && otherCell.id == 281474976710660 && cell.connections
            && cell.connections->size() == 2) {
            int dummy = 0;
        }
        if (cell.id == 281474976710660 && otherCell.id == 281474976710662 && cell.connections
            && cell.connections->size() == 2) {
            int dummy = 0;
        }

        if (cell.connections->empty()) {
            ConnectionDescription newConnection;
            newConnection.cellId = otherCell.id;
            newConnection.distance = (*otherCell.pos - *cell.pos).length();
            newConnection.angleFromPrevious = 360.0f;
            cell.connections->emplace_back(newConnection);
            return;
        }
        if (1 == cell.connections->size()) {
            ConnectionDescription newConnection;
            newConnection.cellId = otherCell.id;
            newConnection.distance = (*otherCell.pos - *cell.pos).length();

            auto connectedCell = getCellRef(cell.connections->front().cellId, cache);
            auto connectedCellDelta = *connectedCell.pos - *cell.pos;
            auto prevAngle = Physics::angleOfVector(connectedCellDelta);
            auto angleDiff = newAngle - prevAngle;
            if (angleDiff >= 0) {
                newConnection.angleFromPrevious = angleDiff;
                cell.connections->begin()->angleFromPrevious = 360.0f - angleDiff;
            } else {
                newConnection.angleFromPrevious = 360.0f + angleDiff;
                cell.connections->begin()->angleFromPrevious = -angleDiff;
            }
            cell.connections->emplace_back(newConnection);
            return;
        }

        auto firstConnectedCell = getCellRef(cell.connections->front().cellId, cache);
        auto firstConnectedCellDelta = *firstConnectedCell.pos - *cell.pos;
        auto angle = Physics::angleOfVector(firstConnectedCellDelta);
        auto connectionIt = ++cell.connections->begin();
        while (true) {
            auto nextAngle = angle + connectionIt->angleFromPrevious;

            if ((angle < newAngle && newAngle <= nextAngle)
                || (angle < (newAngle + 360.0f) && (newAngle + 360.0f) <= nextAngle)) {
                break;
            }

            ++connectionIt;
            if (connectionIt == cell.connections->end()) {
                connectionIt = cell.connections->begin();
            }
            angle = nextAngle;
            if (angle > 360.0f) {
                angle -= 360.0f;
            }
        }

        ConnectionDescription newConnection;
        newConnection.cellId = otherCell.id;
        newConnection.distance = (*otherCell.pos - *cell.pos).length();

        auto angleDiff1 = newAngle - angle;
        if (angleDiff1 < 0) {
            angleDiff1 += 360.0f;
        }
        auto angleDiff2 = connectionIt->angleFromPrevious;

        auto factor = (angleDiff2 != 0) ? angleDiff1 / angleDiff2 : 0.5f;
        newConnection.angleFromPrevious = angleDiff2 * factor;
        connectionIt = cell.connections->insert(connectionIt, newConnection);
        ++connectionIt;
        if (connectionIt == cell.connections->end()) {
            connectionIt = cell.connections->begin();
        }
        connectionIt->angleFromPrevious = angleDiff2 * (1 - factor);
    };

    addConnection(cell1, cell2);
    addConnection(cell2, cell1);

    return *this;
}

QVector2D ClusterDescription::getClusterPosFromCells() const
{
	QVector2D result;
	if (cells) {
		for (CellDescription const& cell : *cells) {
			result += *cell.pos;
		}
		result /= cells->size();
	}
	return result;
}

CellDescription& ClusterDescription::getCellRef(uint64_t const& cellId, std::unordered_map<uint64_t, int>& cache)
{
    auto findResult = cache.find(cellId);
    if (findResult != cache.end()) {
        return cells->at(findResult->second);
    }
    for (int i = 0; i < cells->size(); ++i) {
        auto& cell = cells->at(i);
        if (cell.id == cellId) {
            cache.emplace(cellId, i);
            return cell;
        }
    }
    THROW_NOT_IMPLEMENTED();
}

ParticleDescription::ParticleDescription(ParticleChangeDescription const & change)
{
	id = change.id;
	pos = static_cast<boost::optional<QVector2D>>(change.pos);
	vel = static_cast<boost::optional<QVector2D>>(change.vel);
	energy = static_cast<boost::optional<double>>(change.energy);
	metadata = static_cast<boost::optional<ParticleMetadata>>(change.metadata);
}

QVector2D DataDescription::calcCenter() const
{
	QVector2D result;
	int numEntities = 0;
	if (clusters) {
		for (auto const& cluster : *clusters) {
			if (cluster.cells) {
				for (auto const& cell : *cluster.cells) {
					result += *cell.pos;
					++numEntities;
				}
			}
		}
	}
	if (particles) {
		for (auto const& particle : *particles) {
			result += *particle.pos;
			++numEntities;
		}
	}
	result /= numEntities;
	return result;
}

void DataDescription::shift(QVector2D const & delta)
{
	if (clusters) {
		for (auto & cluster : *clusters) {
			*cluster.pos += delta;
			if (cluster.cells) {
				for (auto & cell : *cluster.cells) {
					*cell.pos += delta;
				}
			}
		}
	}
	if (particles) {
		for (auto & particle : *particles) {
			*particle.pos += delta;
		}
	}
}
