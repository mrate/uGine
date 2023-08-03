// Source:
// https: //github.com/corporateshark/poisson-disk-generator

/**
 * \file PoissonGenerator.h
 * \brief
 *
 * Poisson Disk Points Generator
 *
 * \version 1.3.0
 * \date 14/03/2021
 * \author Sergey Kosarevsky, 2014-2021
 * \author support@linderdaum.com   http://www.linderdaum.com   http://blog.linderdaum.com
 */
#include "Poisson.h"

#include <list>

/*
	Usage example:
		#define POISSON_PROGRESS_INDICATOR 1
		#include "PoissonGenerator.h"
		...
		PoissonGenerator::DefaultPRNG PRNG;
		const auto Points = PoissonGenerator::GeneratePoissonPoints( NumPoints, PRNG );
*/

// Fast Poisson Disk Sampling in Arbitrary Dimensions
// http://people.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf

// Implementation based on http://devmag.org.za/2009/05/03/poisson-disk-sampling/

/* Versions history:
 *		1.3     Mar 14, 2021		Bugfixes: number of points in the !isCircle mode, incorrect loop boundaries
 *		1.2     Dec 28, 2019		Bugfixes; more consistent progress indicator; new command line options in demo app
 *		1.1.6   Dec  7, 2019		Removed duplicate seed initialization; fixed warnings
 *		1.1.5   Jun 16, 2019		In-class initializers; default ctors; naming, shorter code
 *		1.1.4   Oct 19, 2016		POISSON_PROGRESS_INDICATOR can be defined outside of the header file, disabled by default
 *		1.1.3a  Jun  9, 2016		Update constructor for DefaultPRNG
 *		1.1.3   Mar 10, 2016		Header-only library, no global mutable state
 *		1.1.2   Apr  9, 2015		Output a text file with XY coordinates
 *		1.1.1   May 23, 2014		Initialize PRNG seed, fixed uninitialized fields
 *		1.1     May  7, 2014		Support of density maps
 *		1.0     May  6, 2014
*/

#include <random>
#include <stdint.h>
#include <vector>

namespace PoissonGenerator {

const char* Version = "1.3.0 (14/03/2021)";

class DefaultPRNG {
public:
    DefaultPRNG() = default;
    explicit DefaultPRNG(u32 seed)
        : gen_(seed) {}

    f32 randomFloat() { return static_cast<f32>(dis_(gen_)); }

    int randomInt(int maxValue) {
        std::uniform_int_distribution<> disInt(0, maxValue);
        return disInt(gen_);
    }

private:
    std::mt19937 gen_ = std::mt19937(std::random_device()());
    std::uniform_real_distribution<f32> dis_ = std::uniform_real_distribution<f32>(0.0f, 1.0f);
};

struct Point {
    Point() = default;
    Point(f32 X, f32 Y)
        : x(X)
        , y(Y)
        , valid_(true) {}
    f32 x = 0.0f;
    f32 y = 0.0f;
    bool valid_ = false;
    //
    bool isInRectangle() const { return x >= 0 && y >= 0 && x <= 1 && y <= 1; }
    //
    bool isInCircle() const {
        const f32 fx = x - 0.5f;
        const f32 fy = y - 0.5f;
        return (fx * fx + fy * fy) <= 0.25f;
    }
};

struct GridPoint {
    GridPoint() = delete;
    GridPoint(int X, int Y)
        : x(X)
        , y(Y) {}
    int x;
    int y;
};

f32 getDistance(const Point& P1, const Point& P2) {
    return sqrt((P1.x - P2.x) * (P1.x - P2.x) + (P1.y - P2.y) * (P1.y - P2.y));
}

GridPoint imageToGrid(const Point& P, f32 cellSize) {
    return GridPoint((int)(P.x / cellSize), (int)(P.y / cellSize));
}

struct Grid {
    Grid(int w, int h, f32 cellSize)
        : w_(w)
        , h_(h)
        , cellSize_(cellSize) {
        grid_.resize(h_);
        for (auto i = grid_.begin(); i != grid_.end(); i++) {
            i->resize(w);
        }
    }
    void insert(const Point& p) {
        const GridPoint g = imageToGrid(p, cellSize_);
        grid_[g.x][g.y] = p;
    }
    bool isInNeighbourhood(const Point& point, f32 minDist, f32 cellSize) {
        const GridPoint g = imageToGrid(point, cellSize);

        // number of adjucent cells to look for neighbour points
        const int D = 5;

        // scan the neighbourhood of the point in the grid
        for (int i = g.x - D; i <= g.x + D; i++) {
            for (int j = g.y - D; j <= g.y + D; j++) {
                if (i >= 0 && i < w_ && j >= 0 && j < h_) {
                    const Point P = grid_[i][j];

                    if (P.valid_ && getDistance(P, point) < minDist)
                        return true;
                }
            }
        }

        return false;
    }

private:
    int w_;
    int h_;
    f32 cellSize_;
    std::vector<std::vector<Point>> grid_;
};

template <typename PRNG> Point popRandom(std::vector<Point>& points, PRNG& generator) {
    const int idx = generator.randomInt(static_cast<int>(points.size()) - 1);
    const Point p = points[idx];
    points.erase(points.begin() + idx);
    return p;
}

template <typename PRNG> Point generateRandomPointAround(const Point& p, f32 minDist, PRNG& generator) {
    // start with non-uniform distribution
    const f32 R1 = generator.randomFloat();
    const f32 R2 = generator.randomFloat();

    // radius should be between MinDist and 2 * MinDist
    const f32 radius = minDist * (R1 + 1.0f);

    // random angle
    const f32 angle = 2 * 3.141592653589f * R2;

    // the new point is generated around the point (x, y)
    const f32 x = p.x + radius * cos(angle);
    const f32 y = p.y + radius * sin(angle);

    return Point(x, y);
}

/**
	Return a vector of generated points
	NewPointsCount - refer to bridson-siggraph07-poissondisk.pdf for details (the value 'k')
	Circle  - 'true' to fill a circle, 'false' to fill a rectangle
	MinDist - minimal distance estimator, use negative value for default
**/
template <typename PRNG = DefaultPRNG>
std::vector<Point> generatePoissonPoints(size_t numPoints, PRNG& generator, bool isCircle = true, int newPointsCount = 30, f32 minDist = -1.0f) {
    numPoints *= 2;

    // if we want to generate a Poisson square shape, multiply the estimate number of points by PI/4 due to reduced shape area
    if (!isCircle) {
        const f64 Pi_4 = 0.785398163397448309616; // PI/4
        numPoints = static_cast<int>(Pi_4 * numPoints);
    }

    if (minDist < 0.0f) {
        minDist = sqrt(f32(numPoints)) / f32(numPoints);
    }

    std::vector<Point> samplePoints;
    std::vector<Point> processList;

    if (!numPoints)
        return samplePoints;

    // create the grid
    const f32 cellSize = minDist / sqrt(2.0f);

    const int gridW = (int)ceil(1.0f / cellSize);
    const int gridH = (int)ceil(1.0f / cellSize);

    Grid grid(gridW, gridH, cellSize);

    Point firstPoint;
    do {
        firstPoint = Point(generator.randomFloat(), generator.randomFloat());
    } while (!(isCircle ? firstPoint.isInCircle() : firstPoint.isInRectangle()));

    // update containers
    processList.push_back(firstPoint);
    samplePoints.push_back(firstPoint);
    grid.insert(firstPoint);

#if POISSON_PROGRESS_INDICATOR
    size_t progress = 0;
#endif

    // generate new points for each point in the queue
    while (!processList.empty() && samplePoints.size() <= numPoints) {
#if POISSON_PROGRESS_INDICATOR
        // a progress indicator, kind of
        if ((samplePoints.size()) % 1000 == 0) {
            const size_t newProgress = 200 * (samplePoints.size() + processList.size()) / numPoints;
            if (newProgress != progress) {
                progress = newProgress;
                std::cout << ".";
            }
        }
#endif // POISSON_PROGRESS_INDICATOR

        const Point point = popRandom<PRNG>(processList, generator);

        for (int i = 0; i < newPointsCount; i++) {
            const Point newPoint = generateRandomPointAround(point, minDist, generator);
            const bool canFitPoint = isCircle ? newPoint.isInCircle() : newPoint.isInRectangle();

            if (canFitPoint && !grid.isInNeighbourhood(newPoint, minDist, cellSize)) {
                processList.push_back(newPoint);
                samplePoints.push_back(newPoint);
                grid.insert(newPoint);
                continue;
            }
        }
    }

#if POISSON_PROGRESS_INDICATOR
    std::cout << std::endl << std::endl;
#endif // POISSON_PROGRESS_INDICATOR

    return samplePoints;
}

} // namespace PoissonGenerator

namespace ugine::math {

void FastPoissonDisk2D(f32 maxX, f32 maxY, f32 minDistance, std::vector<glm::vec2>& output, u32 maxCount) {
    PoissonGenerator::DefaultPRNG PRNG;
    auto points{ PoissonGenerator::generatePoissonPoints(maxCount, PRNG, false, 30, f32(minDistance) / f32(maxX)) };

    for (auto& point : points) {
        output.push_back(glm::vec2{ maxX * point.x, maxY * point.y });
    }
}

void FastPoissonDisk2D_old(f32 maxX, f32 maxY, f32 minDistance, std::vector<glm::vec2>& output, u32 maxCount) {
    const int k{ 30 };

    const f32 cellSize{ minDistance / std::sqrtf(2.0f) };
    const int gridWidth{ static_cast<int>(std::ceil(f32(maxX) / cellSize)) };
    const int gridHeight{ static_cast<int>(std::ceil(f32(maxY) / cellSize)) };

    std::vector<int> bgGrid(gridWidth * gridHeight, -1);

    glm::vec2 sample{ RandomFloat() * maxX, RandomFloat() * maxY };
    output.push_back(sample);

    std::list<int> activeList;

    int index{ static_cast<int>(output.size() - 1) };

    activeList.push_back(index);
    bgGrid[int(sample.x / cellSize) + int(sample.y / cellSize) * gridWidth] = index;

    while (!activeList.empty()) {
        index = Random(0, static_cast<u32>(activeList.size() - 1));

        auto center{ output[index] };

        bool added{ false };
        for (int i = 0; i < k; ++i) {
            f32 dist{ minDistance + RandomFloat() * minDistance };
            f32 angle{ 2 * PI * RandomFloat() };

            sample = glm::vec2{ std::min<f32>(maxX, std::max(0.0f, center.x + dist * sin(angle))),
                std::min<f32>(maxY, std::max(0.0f, center.y + dist * cos(angle))) };

            int sampleX{ int(sample.x / cellSize) };
            int sampleY{ int(sample.y / cellSize) };

            bool farEnough{ true };
            if (bgGrid[sampleX + sampleY * gridWidth] < 0) {
                for (int x = std::max(0, sampleX - 1); x < std::min(gridWidth - 1, sampleX + 1); ++x) {
                    for (int y = std::max(0, sampleY - 1); y < std::min(gridHeight - 1, sampleY + 1); ++y) {
                        auto bg{ bgGrid[x + y * gridWidth] };
                        if (bg >= 0 && glm::length(output[bg] - sample) < minDistance) {
                            farEnough = false;
                            break;
                        }
                    }
                }
            } else {
                farEnough = false;
            }

            if (farEnough) {
                added = true;
                output.push_back(sample);

                if (maxCount > 0 && output.size() >= maxCount) {
                    return;
                }

                index = static_cast<int>(output.size() - 1);
                bgGrid[sampleX + sampleY * gridWidth] = index;
                activeList.push_back(index);
            }
        }

        if (!added) {
            auto it{ activeList.begin() };
            std::advance(it, index);
            activeList.erase(it);
        }
    }
}

} // namespace ugine::math