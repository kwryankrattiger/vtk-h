#include "PointRenderer.hpp"

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperPoint.h>
#include <vtkh/filters/ParticleMerging.hpp>
#include <memory>

namespace vtkh {

PointRenderer::PointRenderer()
  : m_use_nodes(true),
    m_radius_set(false),
    m_use_variable_radius(false),
    m_base_radius(0.5f),
    m_delta_radius(0.5f),
    m_delete_input(false)
{
  typedef vtkm::rendering::MapperPoint TracerType;
  auto mapper = std::make_shared<TracerType>();
  mapper->SetCompositeBackground(false);
  this->m_mapper = mapper;
}

PointRenderer::~PointRenderer()
{
}

Renderer::vtkmCanvasPtr
PointRenderer::GetNewCanvas(int width, int height)
{
  return std::make_shared<vtkm::rendering::CanvasRayTracer>(width, height);
}

std::string
PointRenderer::GetName() const
{
  return "vtkh::PointRenderer";
}

void
PointRenderer::UseCells()
{
  m_use_nodes = false;
}

void
PointRenderer::UseNodes()
{
  m_use_nodes = true;
}

void
PointRenderer::UseVariableRadius(bool useVariableRadius)
{
  m_use_variable_radius = useVariableRadius;
}

void
PointRenderer::SetBaseRadius(vtkm::Float32 radius)
{
  m_base_radius = radius;
  m_radius_set = true;
}

void
PointRenderer::SetRadiusDelta(vtkm::Float32 delta)
{
  m_delta_radius = delta;
}

void
PointRenderer::PreExecute()
{
  Renderer::PreExecute();

  typedef vtkm::rendering::MapperPoint MapperType;
  std::shared_ptr<MapperType> mesh_mapper =
    std::dynamic_pointer_cast<MapperType>(this->m_mapper);

  if(m_use_nodes)
  {
    mesh_mapper->UseNodes();
  }
  else
  {
    mesh_mapper->UseCells();
  }

  vtkm::Float32 radius = m_base_radius;
  if(m_radius_set)
  {
    mesh_mapper->SetRadius(m_base_radius);
  }
  else
  {
    vtkm::Bounds coordBounds = this->m_input->GetGlobalBounds();
    // set a default radius
    vtkm::Float64 lx = coordBounds.X.Length();
    vtkm::Float64 ly = coordBounds.Y.Length();
    vtkm::Float64 lz = coordBounds.Z.Length();
    vtkm::Float64 mag = vtkm::Sqrt(lx * lx + ly * ly + lz * lz);
    // same as used in vtk ospray
    constexpr vtkm::Float64 heuristic = 1000.;
    radius = static_cast<vtkm::Float32>(mag / heuristic);
    mesh_mapper->SetRadius(radius);
  }

  if(!m_use_nodes)
  {
    vtkm::Float32 max_radius = radius;
    if(m_use_variable_radius)
    {
      max_radius = radius + radius * m_delta_radius;
    }
    std::cout<<"Using max radius of "<<max_radius<<"\n";

    this->m_input->PrintSummary(std::cout);
    ParticleMerging  cleaner;
    cleaner.SetInput(this->m_input);
    cleaner.SetField(this->m_field_name);
    cleaner.SetRadius(max_radius);
    cleaner.Update();
    this->m_input = cleaner.GetOutput();
    std::cout<<"\n\n\n\n";
    this->m_input->PrintSummary(std::cout);
    m_delete_input = true;
  }

  mesh_mapper->UseVariableRadius(m_use_variable_radius);
  mesh_mapper->SetRadiusDelta(m_delta_radius);

}

void PointRenderer::PostExecute()
{
  Renderer::PostExecute();
  if(m_delete_input)
  {
    delete this->m_input;
  }
}

} // namespace vtkh
