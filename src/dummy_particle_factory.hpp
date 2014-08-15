#pragma once

#include <yarrr/particle.hpp>

namespace yarrr
{

class PhysicalParameters;

}

class DummyParticleFactory : public yarrr::ParticleFactory
{
  public:
    virtual ~DummyParticleFactory() = default;
    virtual void create( const yarrr::PhysicalParameters& ) override {}

};

