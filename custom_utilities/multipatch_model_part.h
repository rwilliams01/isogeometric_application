//
//   Project Name:        Kratos
//   Last Modified by:    $Author: hbui $
//   Date:                $Date: 20 Nov 2017 $
//   Revision:            $Revision: 1.0 $
//
//

#if !defined(KRATOS_ISOGEOMETRIC_APPLICATION_MULTIPATCH_MODEL_PART_H_INCLUDED)
#define  KRATOS_ISOGEOMETRIC_APPLICATION_MULTIPATCH_MODEL_PART_H_INCLUDED

// System includes
#include <vector>

// External includes

// Project includes
#include "includes/define.h"
#include "includes/kratos_flags.h"
#include "includes/deprecated_variables.h"
#include "includes/model_part.h"
#include "utilities/openmp_utils.h"
#include "custom_utilities/iga_define.h"
#include "custom_utilities/patch.h"
#include "custom_utilities/control_grid_utility.h"
#include "custom_utilities/multipatch_utility.h"
#include "custom_utilities/nurbs/bcell.h"
#include "custom_utilities/tsplines/tcell.h"
#include "custom_geometries/isogeometric_geometry.h"
#include "isogeometric_application/isogeometric_application.h"

#define ENABLE_PROFILING
// #define DEBUG_GEN_ENTITY

namespace Kratos
{

/**
 * Coupling between KRATOS model_part and multipatch structure
 */
template<int TDim>
class MultiPatchModelPart : public IsogeometricEcho
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(MultiPatchModelPart);

    /// Type definition
    typedef Patch<TDim> PatchType;
    typedef MultiPatch<TDim> MultiPatchType;
    typedef Element::NodeType NodeType;
    typedef IsogeometricGeometry<NodeType> IsogeometricGeometryType;
    typedef typename Patch<TDim>::ControlPointType ControlPointType;
    typedef ModelPart::NodesContainerType NodesContainerType;

    /// Default constructor
    MultiPatchModelPart(typename MultiPatch<TDim>::Pointer pMultiPatch)
    : mpMultiPatch(pMultiPatch), mIsModelPartReady(false)
    {
        mpModelPart = ModelPart::Pointer(new ModelPart("MultiPatch"));
    }

    /// Destructor
    virtual ~MultiPatchModelPart() {}

    /// Get the underlying model_part pointer
    ModelPart::Pointer pModelPart() {return mpModelPart;}

    /// Get the underlying model_part pointer
    ModelPart::ConstPointer pModelPart() const {return mpModelPart;}

    /// Get the underlying multipatch pointer
    typename MultiPatch<TDim>::Pointer pMultiPatch() {return mpMultiPatch;}

    /// Get the underlying multipatch pointer
    typename MultiPatch<TDim>::ConstPointer pMultiPatch() const {return mpMultiPatch;}

    /// Check if the multipatch model_part ready for transferring/transmitting data
    bool IsReady() const {return mpMultiPatch->IsEnumerated() && mIsModelPartReady;}

    /// Start the process to cook new model_part. This function will first create the new model_part instance and add in the nodes (which are the control points in the multipatch)
    void BeginModelPart()
    {
        mIsModelPartReady = false;

        // always enumerate the multipatch first
        mpMultiPatch->Enumerate();

        // create new model_part
        ModelPart::Pointer pNewModelPart = ModelPart::Pointer(new ModelPart(mpModelPart->Name()));

        // swap the internal model_part with new model_part
        mpModelPart.swap(pNewModelPart);
    }

    /// create the nodes from the control points and add to the model_part
    void CreateNodes()
    {
        #ifdef ENABLE_PROFILING
        double start = OpenMPUtils::GetCurrentTime();
        #endif

        if (!mpMultiPatch->IsEnumerated())
            KRATOS_THROW_ERROR(std::logic_error, "The multipatch is not enumerated", "")

        // create new nodes from control points
        for (std::size_t idof = 0; idof < mpMultiPatch->EquationSystemSize(); ++idof)
        {
            std::tuple<std::size_t, std::size_t> loc = mpMultiPatch->EquationIdLocation(idof);

            const std::size_t& patch_id = std::get<0>(loc);
            const std::size_t& local_id = std::get<1>(loc);
            // KRATOS_WATCH(patch_id)
            // KRATOS_WATCH(local_id)

            const ControlPointType& point = mpMultiPatch->pGetPatch(patch_id)->pControlPointGridFunction()->pControlGrid()->GetData(local_id);
//            std::cout << "dof " << idof << ": point " << point << ", patch " << patch_id << std::endl;

            ModelPart::NodeType::Pointer pNewNode = mpModelPart->CreateNewNode(CONVERT_INDEX_IGA_TO_KRATOS(idof), point.X(), point.Y(), point.Z());
            pNewNode->SetValue(NURBS_WEIGHT, point.W());
        }

        if (this->GetEchoLevel() > 0)
        {
            #ifdef ENABLE_PROFILING
            std::cout << "+++ " << __FUNCTION__ << " completed: " << OpenMPUtils::GetCurrentTime() - start << " s" << std::endl;
            #else
            std::cout << __FUNCTION__ << " completed" << std::endl;
            #endif
        }
    }

    /// create the elements out from the patch and add to the model_part
    ModelPart::ElementsContainerType AddElements(typename Patch<TDim>::Pointer pPatch, const std::string& element_name,
            const std::size_t& starting_id, Properties::Pointer pProperties)
    {
        if (IsReady()) return ModelPart::ElementsContainerType(); // call BeginModelPart first before adding elements

        #ifdef ENABLE_PROFILING
        double start = OpenMPUtils::GetCurrentTime();
        #endif

        // get the grid function for control points
        const GridFunction<TDim, ControlPointType>& rControlPointGridFunction = pPatch->ControlPointGridFunction();

        // create new elements and add to the model_part
        ModelPart::ElementsContainerType pNewElements = CreateEntitiesFromFESpace<Element, FESpace<TDim>, ControlGrid<ControlPointType>, ModelPart::NodesContainerType>(pPatch->pFESpace(), rControlPointGridFunction.pControlGrid(), mpModelPart->Nodes(), element_name, starting_id, pProperties, this->GetEchoLevel());

        for (ModelPart::ElementsContainerType::ptr_iterator it = pNewElements.ptr_begin(); it != pNewElements.ptr_end(); ++it)
        {
            mpModelPart->Elements().push_back(*it);
        }

        // sort the element container and make it consistent
        mpModelPart->Elements().Unique();

        if (this->GetEchoLevel() > 0)
        {
            #ifdef ENABLE_PROFILING
            std::cout << "+++ " << __FUNCTION__ << " completed: " << OpenMPUtils::GetCurrentTime() - start << " s, ";
            #else
            std::cout << __FUNCTION__ << " completed, ";
            #endif
            std::cout << pNewElements.size() << " elements of type " << element_name << " are generated for patch " << pPatch->Id() << std::endl;
        }

        return pNewElements;
    }

    /// create the conditions out from the patch and add to the model_part
    ModelPart::ConditionsContainerType AddConditions(typename Patch<TDim>::Pointer pPatch, const std::string& condition_name,
            const std::size_t& starting_id, Properties::Pointer pProperties)
    {
        if (IsReady()) return ModelPart::ConditionsContainerType(); // call BeginModelPart first before adding conditions

        #ifdef ENABLE_PROFILING
        double start = OpenMPUtils::GetCurrentTime();
        #endif

        // get the grid function for control points
        const GridFunction<TDim, ControlPointType>& rControlPointGridFunction = pPatch->ControlPointGridFunction();

        // create new elements and add to the model_part
        ModelPart::ConditionsContainerType pNewConditions = CreateEntitiesFromFESpace<Condition, FESpace<TDim>, ControlGrid<ControlPointType>, ModelPart::NodesContainerType>(pPatch->pFESpace(), rControlPointGridFunction.pControlGrid(), mpModelPart->Nodes(), condition_name, starting_id, pProperties, this->GetEchoLevel());

        for (ModelPart::ConditionsContainerType::ptr_iterator it = pNewConditions.ptr_begin(); it != pNewConditions.ptr_end(); ++it)
        {
            mpModelPart->Conditions().push_back(*it);
        }

        // sort the element container and make it consistent
        mpModelPart->Conditions().Unique();

        if (this->GetEchoLevel() > 0)
        {
            #ifdef ENABLE_PROFILING
            std::cout << "+++ " << __FUNCTION__ << " completed: " << OpenMPUtils::GetCurrentTime() - start << " s, ";
            #else
            std::cout << __FUNCTION__ << " completed, ";
            #endif
            std::cout << pNewConditions.size() << " conditions of type " << condition_name << " are generated for patch " << pPatch->Id() << std::endl;
        }

        return pNewConditions;
    }

    /// create the conditions out from the boundary of the patch and add to the model_part
    ModelPart::ConditionsContainerType AddConditions(typename Patch<TDim>::Pointer pPatch, const BoundarySide& side,
            const std::string& condition_name, const std::size_t& starting_id, Properties::Pointer pProperties)
    {
        if (IsReady()) return ModelPart::ConditionsContainerType(); // call BeginModelPart first before adding conditions

        // construct the boundary patch
        typename Patch<TDim-1>::Pointer pBoundaryPatch = pPatch->ConstructBoundaryPatch(side);

        return AddConditions(pBoundaryPatch, condition_name, starting_id, pProperties);
    }

    /// create the conditions out from a boundary patch and add to the model_part
    ModelPart::ConditionsContainerType AddConditions(typename Patch<TDim-1>::Pointer pBoundaryPatch,
            const std::string& condition_name, const std::size_t& starting_id, Properties::Pointer pProperties)
    {
        if (IsReady()) return ModelPart::ConditionsContainerType(); // call BeginModelPart first before adding conditions

        #ifdef ENABLE_PROFILING
        double start = OpenMPUtils::GetCurrentTime();
        #endif

        #ifdef DEBUG_GEN_ENTITY
        KRATOS_WATCH(*pBoundaryPatch)
        #endif

        // get the grid function for control points
        const GridFunction<TDim-1, ControlPointType>& rControlPointGridFunction = pBoundaryPatch->ControlPointGridFunction();

        // create new conditions and add to the model_part
        ModelPart::ConditionsContainerType pNewConditions = CreateEntitiesFromFESpace<Condition, FESpace<TDim-1>, ControlGrid<ControlPointType>, ModelPart::NodesContainerType>(pBoundaryPatch->pFESpace(), rControlPointGridFunction.pControlGrid(), mpModelPart->Nodes(), condition_name, starting_id, pProperties, this->GetEchoLevel());

        // std::cout << "model_part nodes:" << std::endl;
        // for(ModelPart::NodeIterator i = mpModelPart->NodesBegin() ; i != mpModelPart->NodesEnd() ; i++)
        //     std::cout << " " << i->Id() << ": " << i->X0() << " " << i->Y0() << " " << i->Z0() << std::endl;

        for (ModelPart::ConditionsContainerType::ptr_iterator it = pNewConditions.ptr_begin(); it != pNewConditions.ptr_end(); ++it)
        {
            mpModelPart->Conditions().push_back(*it);

            // std::cout << "condition nodes:" << std::endl;
            // for (std::size_t i = 0; i < (*it)->GetGeometry().size(); ++i)
            //     std::cout << " " << (*it)->GetGeometry()[i].Id() << ":"
            //               << " " << (*it)->GetGeometry()[i].X0()
            //               << " " << (*it)->GetGeometry()[i].Y0()
            //               << " " << (*it)->GetGeometry()[i].Z0()
            //               << std::endl;
        }

        // sort the condition container and make it consistent
        mpModelPart->Conditions().Unique();

        if (this->GetEchoLevel() > 0)
        {
            #ifdef ENABLE_PROFILING
            std::cout << "+++ " << __FUNCTION__ << " completed: " << OpenMPUtils::GetCurrentTime() - start << " s, ";
            #else
            std::cout << __FUNCTION__ << " completed, ";
            #endif
            std::cout << pNewConditions.size() << " conditions of type " << condition_name << " are generated for boundary patch " << pBoundaryPatch->Id() << std::endl;
        }

        return pNewConditions;
    }

    /// Finalize the model_part creation process
    void EndModelPart()
    {
        if (IsReady()) return;
        mIsModelPartReady = true;
    }

    /// Synchronize from multipatch to model_part
    template<class TVariableType>
    void SynchronizeForward(const TVariableType& rVariable)
    {
        if (!IsReady()) return;

        if (!mpMultiPatch->IsEnumerated())
            KRATOS_THROW_ERROR(std::logic_error, "The multipatch is not enumerated", "")

        // transfer data from from control points to nodes
        for (std::size_t idof = 0; idof < mpMultiPatch->EquationSystemSize(); ++idof)
        {
            std::tuple<std::size_t, std::size_t> loc = mpMultiPatch->EquationIdLocation(idof);

            const std::size_t& patch_id = std::get<0>(loc);
            const std::size_t& local_id = std::get<1>(loc);
            // KRATOS_WATCH(patch_id)
            // KRATOS_WATCH(local_id)

            const typename TVariableType::Type& value = mpMultiPatch->pGetPatch(patch_id)->pGetGridFunction(rVariable)->pControlGrid()->GetData(local_id);
            // KRATOS_WATCH(value)

            ModelPart::NodeType::Pointer pNode = mpModelPart->pGetNode(CONVERT_INDEX_IGA_TO_KRATOS(idof));

            pNode->GetSolutionStepValue(rVariable) = value;
        }
    }

    /// Synchronize from model_part to the multipatch
    template<class TVariableType>
    void SynchronizeBackward(const TVariableType& rVariable)
    {
        if (!IsReady()) return;

        // loop through each patch, we construct a map from each function id to the patch id
        typedef typename MultiPatch<TDim>::patch_iterator patch_iterator;
        for (patch_iterator it = mpMultiPatch->begin();
                it != mpMultiPatch->end(); ++it)
        {
            std::vector<std::size_t> func_ids = it->pFESpace()->FunctionIndices();

            // check if the grid function existed in the patch
            if (!it->template HasGridFunction<TVariableType>(rVariable))
            {
                // --> if not then create the new grid function
                typename ControlGrid<typename TVariableType::Type>::Pointer pNewControlGrid = ControlGridUtility::CreateControlGrid<TDim, TVariableType>(it->pFESpace(), rVariable);
                it->template CreateGridFunction<TVariableType>(rVariable, pNewControlGrid);
            }

            // get the control grid
            typename ControlGrid<typename TVariableType::Type>::Pointer pControlGrid = it->pGetGridFunction(rVariable)->pControlGrid();

            // set the data for the control grid
            for (std::size_t i = 0; i < pControlGrid->size(); ++i)
            {
                std::size_t global_id = func_ids[i];
                std::size_t node_id = CONVERT_INDEX_IGA_TO_KRATOS(global_id);

                NodesContainerType::iterator it_node = mpModelPart->Nodes().find(node_id);
                if (it_node == mpModelPart->Nodes().end())
                {
                    std::stringstream ss;
                    ss << "Node " << node_id << " does not exist in the model_part " << mpModelPart->Name();
                    KRATOS_THROW_ERROR(std::logic_error, ss.str(), "")
                }
                pControlGrid->SetData(i, it_node->GetSolutionStepValue(rVariable));
            }
        }
    }

    /// Create entities (elements/conditions) from FESpace
    /// @param pFESpace the finite element space to provide the cell manager
    /// @param pControlPointGrid control grid to provide control points
    /// @param rNodes model_part Nodes to look up for when creating elements
    /// @param element_name name of the sample element
    /// @param starting_id the first id of the newly created entities, from there the id is incremental
    /// @param p_temp_properties the Properties to create new entities
    template<class TEntityType, class TFESpace, class TControlGridType, class TNodeContainerType>
    static PointerVectorSet<TEntityType, IndexedObject> CreateEntitiesFromFESpace(typename TFESpace::ConstPointer pFESpace,
        typename TControlGridType::ConstPointer pControlPointGrid,
        TNodeContainerType& rNodes, const std::string& element_name,
        const std::size_t& starting_id, Properties::Pointer p_temp_properties,
        const int& echo_level)
    {
        #ifdef ENABLE_PROFILING
        double start = OpenMPUtils::GetCurrentTime();
        #endif

        // construct the cell manager out from the FESpace
        typedef typename TFESpace::cell_container_t cell_container_t;
        typename cell_container_t::Pointer pCellManager = pFESpace->ConstructCellManager();

        if (echo_level > 0)
        {
            #ifdef ENABLE_PROFILING
            std::cout << "  ++ ConstructCellManager: " << OpenMPUtils::GetCurrentTime()-start << " s" << std::endl;
            start = OpenMPUtils::GetCurrentTime();
            #endif
        }

        // container for newly created elements
        PointerVectorSet<TEntityType, IndexedObject> pNewElements;

        // get the sample element
        if(!KratosComponents<TEntityType>::Has(element_name))
        {
            std::stringstream buffer;
            buffer << "Entity (Element/Condition) " << element_name << " is not registered in Kratos.";
            KRATOS_THROW_ERROR(std::invalid_argument, buffer.str(), "");
            return pNewElements;
        }

        TEntityType const& r_clone_element = KratosComponents<TEntityType>::Get(element_name);

        // loop through each cell in the space
        typename TEntityType::NodesArrayType temp_element_nodes;
        typename IsogeometricGeometryType::Pointer p_temp_geometry;
        std::size_t cnt = starting_id;
        Vector dummy;
        int max_integration_method = 1;
        if (p_temp_properties->Has(NUM_IGA_INTEGRATION_METHOD))
            max_integration_method = (*p_temp_properties)[NUM_IGA_INTEGRATION_METHOD];

        for (typename cell_container_t::iterator it_cell = pCellManager->begin(); it_cell != pCellManager->end(); ++it_cell)
        {
            // KRATOS_WATCH(*(*it_cell))
            // get new nodes
            temp_element_nodes.clear();

            const std::vector<std::size_t>& anchors = (*it_cell)->GetSupportedAnchors();
            Vector weights(anchors.size());
            for (std::size_t i = 0; i < anchors.size(); ++i)
            {
                temp_element_nodes.push_back(( *(MultiPatchUtility::FindKey(rNodes, CONVERT_INDEX_IGA_TO_KRATOS(anchors[i]), "Node").base())));
                weights[i] = pControlPointGrid->GetData(pFESpace->LocalId(anchors[i])).W();
            }

            if (echo_level > 1)
            {
                std::cout << "anchors:";
                for (std::size_t i = 0; i < anchors.size(); ++i)
                    std::cout << " " << CONVERT_INDEX_IGA_TO_KRATOS(anchors[i]);
                std::cout << std::endl;
                KRATOS_WATCH(weights)
                // KRATOS_WATCH((*it_cell)->GetExtractionOperator())
                KRATOS_WATCH((*it_cell)->GetCompressedExtractionOperator())
                KRATOS_WATCH(pFESpace->Order(0))
                KRATOS_WATCH(pFESpace->Order(1))
                KRATOS_WATCH(pFESpace->Order(2))
            }

            // create the geometry
            p_temp_geometry = boost::dynamic_pointer_cast<IsogeometricGeometryType>(r_clone_element.GetGeometry().Create(temp_element_nodes));
            if (p_temp_geometry == NULL)
                KRATOS_THROW_ERROR(std::runtime_error, "The cast to IsogeometricGeometry is failed.", "")

            p_temp_geometry->AssignGeometryData(dummy,
                                                dummy,
                                                dummy,
                                                weights,
                                                // (*it_cell)->GetExtractionOperator(),
                                                (*it_cell)->GetCompressedExtractionOperator(),
                                                static_cast<int>(pFESpace->Order(0)),
                                                static_cast<int>(pFESpace->Order(1)),
                                                static_cast<int>(pFESpace->Order(2)),
                                                max_integration_method);

            if (echo_level > 1)
            {
                for (int irule = 0; irule < max_integration_method; ++irule)
                {
                    std::cout << "integration points for rule " << irule << ":" << std::endl;
                    typedef typename IsogeometricGeometryType::IntegrationPointsArrayType IntegrationPointsArrayType;
                    const IntegrationPointsArrayType& integration_points = p_temp_geometry->IntegrationPoints((GeometryData::IntegrationMethod) irule);
                    for (std::size_t i = 0; i < integration_points.size(); ++i)
                        std::cout << " " << i << ": " << integration_points[i] << std::endl;
                }
            }

            // create the element and add to the list
            typename TEntityType::Pointer pNewElement = r_clone_element.Create(cnt++, p_temp_geometry, p_temp_properties);
            pNewElement->SetValue(ACTIVATION_LEVEL, 0);
            #ifdef IS_INACTIVE
            pNewElement->SetValue(IS_INACTIVE, false);
            #endif
            pNewElement->Set(ACTIVE, true);
            pNewElements.push_back(pNewElement);

            //////////
            try
            {
                BCell& c = dynamic_cast<BCell&>(**it_cell);
                pNewElement->SetValue( KNOT_LEFT, c.XiMinValue() );
                pNewElement->SetValue( KNOT_RIGHT, c.XiMaxValue() );
                pNewElement->SetValue( KNOT_BOTTOM, c.EtaMinValue() );
                pNewElement->SetValue( KNOT_TOP, c.EtaMaxValue() );
                pNewElement->SetValue( KNOT_FRONT, c.ZetaMinValue() );
                pNewElement->SetValue( KNOT_BACK, c.ZetaMaxValue() );
            }
            catch (std::bad_cast& bc)
            {
                if (echo_level > 2)
                    std::cout << "WARNING: cell " << (*it_cell)->Id() << " cannot be casted to BCell" << std::endl;
            }

            try
            {
                TCell& c = dynamic_cast<TCell&>(**it_cell);
                pNewElement->SetValue( KNOT_LEFT, c.XiMinValue() );
                pNewElement->SetValue( KNOT_RIGHT, c.XiMaxValue() );
                pNewElement->SetValue( KNOT_BOTTOM, c.EtaMinValue() );
                pNewElement->SetValue( KNOT_TOP, c.EtaMaxValue() );
                pNewElement->SetValue( KNOT_FRONT, c.ZetaMinValue() );
                pNewElement->SetValue( KNOT_BACK, c.ZetaMaxValue() );
            }
            catch (std::bad_cast& bc)
            {
                if (echo_level > 2)
                    std::cout << "WARNING: cell " << (*it_cell)->Id() << " cannot be casted to TCell" << std::endl;
            }
            //////////

            // set the level
            pNewElement->SetValue(HIERARCHICAL_LEVEL, (*it_cell)->Level());
            pNewElement->SetValue(CELL_INDEX, (*it_cell)->Id());

            if (echo_level > 1)
            {
                std::cout << "Entity " << element_name << " " << pNewElement->Id() << " is created" << std::endl;
                std::cout << "  Connectivity:";
                for (unsigned int i = 0; i < p_temp_geometry->size(); ++i)
                {
                    std::cout << " " << (*p_temp_geometry)[i].Id();
                }
                std::cout << std::endl;
            }
        }

        if (echo_level > 0)
        {
            #ifdef ENABLE_PROFILING
            std::cout << "  ++ generate " << r_clone_element.Info() << " entities: " << OpenMPUtils::GetCurrentTime()-start << " s" << std::endl;
            start = OpenMPUtils::GetCurrentTime();
            #endif
        }

        return pNewElements;
    }

    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << "MultiPatchModelPart";
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
        rOStream << "+++ModelPart:" << std::endl;
        rOStream << *mpModelPart << std::endl;
        rOStream << "+++MultiPatch" << std::endl;
        rOStream << *mpMultiPatch << std::endl;
    }

private:

    bool mIsModelPartReady;

    ModelPart::Pointer mpModelPart;
    typename MultiPatch<TDim>::Pointer mpMultiPatch;

};

/// output stream function
template<int TDim>
inline std::ostream& operator <<(std::ostream& rOStream, const MultiPatchModelPart<TDim>& rThis)
{
    rThis.PrintInfo(rOStream);
    rOStream << std::endl;
    rThis.PrintData(rOStream);
    return rOStream;
}

} // namespace Kratos.

#ifdef ENABLE_MORE_OUTPUT
#undef ENABLE_MORE_OUTPUT
#endif

#ifdef DEBUG_GEN_ENTITY
#undef DEBUG_GEN_ENTITY
#endif

#ifdef ENABLE_PROFILING
#undef ENABLE_PROFILING
#endif

#endif // KRATOS_ISOGEOMETRIC_APPLICATION_MULTIPATCH_MODEL_PART_H_INCLUDED

