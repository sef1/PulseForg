// Standalone unit tests for the transport-to-step mapping.
// Build (no JUCE needed):
//   g++ -std=c++17 -Wall -Wextra -o TransportSyncTests plugin/Tests/TransportSyncTests.cpp && ./TransportSyncTests

#include "../Source/TransportSync.h"

#include <cassert>
#include <cmath>
#include <cstdio>

using pulseforge::TransportStep;

int main()
{
    // Bar starts and step boundaries
    assert (TransportStep::stepIndexForPpq (0.0) == 0);
    assert (TransportStep::stepIndexForPpq (0.2499999) == 0);
    assert (TransportStep::stepIndexForPpq (0.25) == 1);
    assert (TransportStep::stepIndexForPpq (0.9999) == 3);
    assert (TransportStep::stepIndexForPpq (1.0) == 4);
    assert (TransportStep::stepIndexForPpq (3.9999) == 15);

    // Bar wrap
    assert (TransportStep::stepIndexForPpq (4.0) == 0);
    assert (TransportStep::stepIndexForPpq (4.25) == 1);
    assert (TransportStep::stepIndexForPpq (39.75) == 15);
    assert (TransportStep::stepIndexForPpq (40.0) == 0);

    // Float noise just under a boundary snaps onto the boundary, never skips a step
    assert (TransportStep::stepIndexForPpq (0.25 - 1e-12) == 1);

    // Negative PPQ (host preroll): floor-mod, not C truncation
    assert (TransportStep::stepIndexForPpq (-0.25) == 15);
    assert (TransportStep::stepIndexForPpq (-1.0) == 12);
    assert (TransportStep::stepIndexForPpq (-4.0) == 0);
    assert (TransportStep::stepIndexForPpq (-4.25) == 15);

    // Phase within step
    assert (std::abs (TransportStep::phaseInStep (0.3) - 0.2) < 1e-9);
    assert (std::abs (TransportStep::phaseInStep (0.0) - 0.0) < 1e-12);
    assert (std::abs (TransportStep::phaseInStep (-0.1) - 0.6) < 1e-9);
    assert (TransportStep::phaseInStep (0.25) < 1e-6);

    std::puts ("TransportSync tests passed");
    return 0;
}
