//
//   Project Name:        Kratos
//   Last Modified by:    $Author: hbui $
//   Date:                $Date: 18 Aug 2013 $
//   Revision:            $Revision: 1.1 $
//
//


// System includes


// External includes


// Project includes
#include "includes/define.h"
#include "includes/serializer.h"
#include "includes/constitutive_law.h"
#include "geometries/geometry.h"
#include "isogeometric_application.h"
#include "custom_geometries/geo_1d_bezier.h"
#include "custom_geometries/geo_2d_bezier.h"
#include "custom_geometries/geo_2d_bezier_3.h"
#include "custom_geometries/geo_3d_bezier.h"


namespace Kratos
{

    KRATOS_CREATE_VARIABLE( double, NURBS_WEIGHT )
    KRATOS_CREATE_VARIABLE( Vector, NURBS_WEIGHTS )
    KRATOS_CREATE_VARIABLE( Vector, NURBS_KNOTS_1 )
    KRATOS_CREATE_VARIABLE( Vector, NURBS_KNOTS_2 )
    KRATOS_CREATE_VARIABLE( Vector, NURBS_KNOTS_3 )
    KRATOS_CREATE_VARIABLE( int, NURBS_DEGREE_1 )
    KRATOS_CREATE_VARIABLE( int, NURBS_DEGREE_2 )
    KRATOS_CREATE_VARIABLE( int, NURBS_DEGREE_3 )
    KRATOS_CREATE_VARIABLE( int, NURBS_DIMENSION_1 )
    KRATOS_CREATE_VARIABLE( int, NURBS_DIMENSION_2 )
    KRATOS_CREATE_VARIABLE( int, NURBS_DIMENSION_3 )
    KRATOS_CREATE_VARIABLE( int, NUM_DIVISION_1 )
    KRATOS_CREATE_VARIABLE( int, NUM_DIVISION_2 )
    KRATOS_CREATE_VARIABLE( int, NUM_DIVISION_3 )
    KRATOS_CREATE_VARIABLE( int, NUM_IGA_INTEGRATION_METHOD )
    KRATOS_CREATE_VARIABLE( Matrix, EXTRACTION_OPERATOR )
    KRATOS_CREATE_VARIABLE( Matrix, EXTRACTION_OPERATOR_MCSR )
    KRATOS_CREATE_VARIABLE( Vector, EXTRACTION_OPERATOR_CSR_ROWPTR )
    KRATOS_CREATE_VARIABLE( Vector, EXTRACTION_OPERATOR_CSR_COLIND )
    KRATOS_CREATE_VARIABLE( Vector, EXTRACTION_OPERATOR_CSR_VALUES )
    KRATOS_CREATE_VARIABLE( ControlPoint<double>, CONTROL_POINT )

    KRATOS_CREATE_3D_VARIABLE_WITH_COMPONENTS( LOCAL_COORDINATES )
    KRATOS_CREATE_3D_VARIABLE_WITH_COMPONENTS( CONTROL_POINT_COORDINATES )

    KRATOS_CREATE_VARIABLE( double, KNOT_LEFT )
    KRATOS_CREATE_VARIABLE( double, KNOT_RIGHT )
    KRATOS_CREATE_VARIABLE( double, KNOT_TOP )
    KRATOS_CREATE_VARIABLE( double, KNOT_BOTTOM )
    KRATOS_CREATE_VARIABLE( double, KNOT_FRONT )
    KRATOS_CREATE_VARIABLE( double, KNOT_BACK )

    KRATOS_CREATE_VARIABLE( int, PATCH_INDEX )
    KRATOS_CREATE_VARIABLE( int, HIERARCHICAL_LEVEL )
    KRATOS_CREATE_VARIABLE( int, BASIS_FUNCTION_INDEX )
    KRATOS_CREATE_VARIABLE( int, EQUATION_INDEX )
    KRATOS_CREATE_VARIABLE( int, CELL_INDEX )

    KratosIsogeometricApplication::KratosIsogeometricApplication()
    : mDummyElementBezier( 0, Element::GeometryType::Pointer( new Geo1dBezier<Node<3> >() ) )
    , mDummyElementBezier2D( 0, Element::GeometryType::Pointer( new Geo2dBezier<Node<3> >() ) )
    , mDummyElementBezier3D( 0, Element::GeometryType::Pointer( new Geo3dBezier<Node<3> >() ) )
    , mDummyConditionBezier( 0, Element::GeometryType::Pointer( new Geo1dBezier<Node<3> >() ) )
    , mDummyConditionBezier2D( 0, Element::GeometryType::Pointer( new Geo2dBezier<Node<3> >() ) )
    , mDummyConditionBezier2D3( 0, Element::GeometryType::Pointer( new Geo2dBezier3<Node<3> >() ) )
    , mDummyConditionBezier3D( 0, Element::GeometryType::Pointer( new Geo3dBezier<Node<3> >() ) )
    {}

    void KratosIsogeometricApplication::Register()
    {
        // calling base class to register Kratos components
        KratosApplication::Register();
        std::cout << "Initializing KratosIsogeometricApplication... " << std::endl;

        //register variables
        KRATOS_REGISTER_VARIABLE( NURBS_WEIGHT )
        KRATOS_REGISTER_VARIABLE( NURBS_WEIGHTS )
        KRATOS_REGISTER_VARIABLE( NURBS_KNOTS_1 )
        KRATOS_REGISTER_VARIABLE( NURBS_KNOTS_2 )
        KRATOS_REGISTER_VARIABLE( NURBS_KNOTS_3 )
        KRATOS_REGISTER_VARIABLE( NURBS_DEGREE_1 )
        KRATOS_REGISTER_VARIABLE( NURBS_DEGREE_2 )
        KRATOS_REGISTER_VARIABLE( NURBS_DEGREE_3 )
        KRATOS_REGISTER_VARIABLE( NURBS_DIMENSION_1 )
        KRATOS_REGISTER_VARIABLE( NURBS_DIMENSION_2 )
        KRATOS_REGISTER_VARIABLE( NURBS_DIMENSION_3 )
        KRATOS_REGISTER_VARIABLE( NUM_DIVISION_1 )
        KRATOS_REGISTER_VARIABLE( NUM_DIVISION_2 )
        KRATOS_REGISTER_VARIABLE( NUM_DIVISION_3 )
        KRATOS_REGISTER_VARIABLE( NUM_IGA_INTEGRATION_METHOD )
        KRATOS_REGISTER_VARIABLE( EXTRACTION_OPERATOR )
        KRATOS_REGISTER_VARIABLE( EXTRACTION_OPERATOR_MCSR )
        KRATOS_REGISTER_VARIABLE( EXTRACTION_OPERATOR_CSR_ROWPTR )
        KRATOS_REGISTER_VARIABLE( EXTRACTION_OPERATOR_CSR_COLIND )
        KRATOS_REGISTER_VARIABLE( EXTRACTION_OPERATOR_CSR_VALUES )
        KRATOS_REGISTER_VARIABLE( CONTROL_POINT )
        KRATOS_REGISTER_VARIABLE( KNOT_LEFT )
        KRATOS_REGISTER_VARIABLE( KNOT_RIGHT )
        KRATOS_REGISTER_VARIABLE( KNOT_TOP )
        KRATOS_REGISTER_VARIABLE( KNOT_BOTTOM )
        KRATOS_REGISTER_VARIABLE( KNOT_FRONT )
        KRATOS_REGISTER_VARIABLE( KNOT_BACK )
        KRATOS_REGISTER_VARIABLE( PATCH_INDEX )
        KRATOS_REGISTER_VARIABLE( HIERARCHICAL_LEVEL )
        KRATOS_REGISTER_VARIABLE( BASIS_FUNCTION_INDEX )
        KRATOS_REGISTER_VARIABLE( EQUATION_INDEX )
        KRATOS_REGISTER_VARIABLE( CELL_INDEX )

        // to make sure the variable imported from other application is registered
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS( LOCAL_COORDINATES )
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS( CONTROL_POINT_COORDINATES )

        // register the geometries
        Geo1dBezier<Node<3> > Geo1dBezierPrototype;
        Serializer::Register( "Geo1dBezier", Geo1dBezierPrototype );

        Geo2dBezier<Node<3> > Geo2dBezierPrototype;
        Serializer::Register( "Geo2dBezier", Geo2dBezierPrototype );

        Geo2dBezier3<Node<3> > Geo2dBezier3Prototype;
        Serializer::Register( "Geo2dBezier3", Geo2dBezier3Prototype );

        Geo3dBezier<Node<3> > Geo3dBezierPrototype;
        Serializer::Register( "Geo3dBezier", Geo3dBezierPrototype );

        // register elements
        KRATOS_REGISTER_ELEMENT( "DummyElementBezier", mDummyElementBezier )
        KRATOS_REGISTER_ELEMENT( "DummyElementBezier2D", mDummyElementBezier2D )
        KRATOS_REGISTER_ELEMENT( "DummyElementBezier3D", mDummyElementBezier3D )

        // register conditions
        KRATOS_REGISTER_CONDITION( "DummyConditionBezier", mDummyConditionBezier )
        KRATOS_REGISTER_CONDITION( "DummyConditionBezier2D", mDummyConditionBezier2D )
        KRATOS_REGISTER_CONDITION( "DummyConditionBezier2D3", mDummyConditionBezier2D3 )
        KRATOS_REGISTER_CONDITION( "DummyConditionBezier3D", mDummyConditionBezier3D )
    }

}  // namespace Kratos.


