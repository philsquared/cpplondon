/**************************************************************************************************
*
* \file Strategy_Benchmark.cpp
* \brief C++ Training - Programming Task for the Strategy Design Pattern
*
* Copyright (C) 2015-2020 Klaus Iglberger - All Rights Reserved
*
* This file is part of the C++ training by Klaus Iglberger. The file may only be used in the
* context of the C++ training or with explicit agreement by Klaus Iglberger.
*
**************************************************************************************************/

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <vector>


struct Vector3D
{
   double x{};
   double y{};
   double z{};
};

Vector3D operator+( const Vector3D& a, const Vector3D& b )
{
   return Vector3D{ a.x+b.x, a.y+b.y, a.z+b.z };
}


namespace classic_solution {

   struct Circle;
   struct Square;

   struct TranslateStrategy
   {
      virtual ~TranslateStrategy() {}

      virtual void translate( Circle& circle, const Vector3D& v ) const = 0;
      virtual void translate( Square& square, const Vector3D& v ) const = 0;
   };


   struct Shape
   {
      Shape() = default;

      virtual ~Shape() {}

      virtual void translate( const Vector3D& v ) = 0;
   };


   struct Circle : public Shape
   {
      Circle( double r, std::unique_ptr<TranslateStrategy>&& ts )
         : radius( r )
         , strategy( std::move(ts) )
      {}

      ~Circle() {}

      void translate( const Vector3D& v ) override { strategy->translate( *this, v ); }

      double radius;
      Vector3D center{};
      std::unique_ptr<TranslateStrategy> strategy;
   };


   struct Square : public Shape
   {
      Square( double s, std::unique_ptr<TranslateStrategy>&& ts )
         : side( s )
         , strategy( std::move(ts) )
      {}

      ~Square() {}

      void translate( const Vector3D& v ) override { strategy->translate( *this, v ); }

      double side;
      Vector3D center{};
      std::unique_ptr<TranslateStrategy> strategy;
   };


   struct ConcreteTranslateStrategy : public TranslateStrategy
   {
      virtual ~ConcreteTranslateStrategy() {}

      void translate( Circle& circle, const Vector3D& v ) const override
      {
         circle.center = circle.center + v;
      }

      void translate( Square& square, const Vector3D& v ) const override
      {
         square.center = square.center + v;
      }
   };

   using Shapes = std::vector< std::unique_ptr<Shape> >;

   void translate( Shapes& shapes, const Vector3D& v )
   {
      for( auto& shape : shapes )
      {
         shape->translate( v );
      }
   }

} // namespace classic_solution


namespace std_function_solution {

   struct Shape
   {
      Shape() = default;

      virtual ~Shape() {}

      virtual void translate( const Vector3D& v ) = 0;
   };


   struct Circle : public Shape
   {
      using TranslateStrategy = std::function<void(Circle&, const Vector3D&)>;

      Circle( double r, TranslateStrategy ts )
         : radius( r )
         , strategy( std::move(ts) )
      {}

      ~Circle() {}

      void translate( const Vector3D& v ) override { strategy( *this, v ); }

      double radius;
      Vector3D center;
      TranslateStrategy strategy;
   };

   void translate( Circle& circle, const Vector3D& v )
   {
      circle.center = circle.center + v;
   }


   struct Square : public Shape
   {
      using TranslateStrategy = std::function<void(Square&, const Vector3D&)>;

      Square( double s, TranslateStrategy ts )
         : side( s )
         , strategy( std::move(ts) )
      {}

      ~Square() {}

      void translate( const Vector3D& v ) override { strategy( *this, v ); }

      double side;
      Vector3D center;
      TranslateStrategy strategy;
   };

   void translate( Square& square, const Vector3D& v )
   {
      square.center = square.center + v;
   }


   struct Translate {
      template< typename T >
      void operator()( T& t, const Vector3D& v )
      {
         translate( t, v );
      }
   };


   using Shapes = std::vector< std::unique_ptr<Shape> >;

   void translate( Shapes& shapes, const Vector3D& v )
   {
      for( auto& shape : shapes )
      {
         shape->translate( v );
      }
   }

} // namespace std_function_solution


namespace manual_function_solution {

   template< typename Fn, size_t N >
   class Function;

   template< typename R, typename... Args, size_t N >
   class Function<R(Args...),N>
   {
    public:
      template< typename Fn >
      Function( Fn fn )
         : pimpl_{ reinterpret_cast<Concept*>( buffer ) }
      {
         static_assert( sizeof(Fn) <= N, "Given type is too large" );
         new (pimpl_) Model<Fn>( fn );
      }

      Function( Function const& f )
         : pimpl_{ reinterpret_cast<Concept*>( buffer ) }
      {
         f.pimpl_->clone( pimpl_ );
      }

      Function& operator=( Function f )
      {
         pimpl_->~Concept();
         f.pimpl_->clone( pimpl_ );
         return *this;
      }

      ~Function() { pimpl_->~Concept(); }

      R operator()( Args... args ) { return (*pimpl_)( std::forward<Args>( args )... ); }

    private:
      class Concept
      {
       public:
         virtual ~Concept() = default;
         virtual R operator()( Args... ) const = 0;
         virtual void clone( Concept* memory ) const = 0;
      };

      template< typename Fn >
      class Model : public Concept
      {
       public:
         explicit Model( Fn fn )
            : fn_( fn )
         {}

         R operator()( Args... args ) const override { return fn_( std::forward<Args>( args )... ); }
         void clone( Concept* memory ) const override { new (memory) Model( fn_ ); }

       private:
         Fn fn_;
      };

      Concept* pimpl_;

      char buffer[N+8UL];
   };


   struct Shape
   {
      Shape() = default;

      virtual ~Shape() {}

      virtual void translate( const Vector3D& v ) = 0;
   };


   struct Circle : public Shape
   {
      using TranslateStrategy = Function<void(Circle&, const Vector3D&),8UL>;

      Circle( double r, TranslateStrategy ts )
         : radius{ r }
         , strategy{ std::move(ts) }
      {}

      ~Circle() {}

      void translate( const Vector3D& v ) override { strategy( *this, v ); }

      double radius;
      Vector3D center;
      TranslateStrategy strategy;
   };

   void translate( Circle& circle, const Vector3D& v )
   {
      circle.center = circle.center + v;
   }


   struct Square : public Shape
   {
      using TranslateStrategy = Function<void(Square&, const Vector3D&),8UL>;

      Square( double s, TranslateStrategy ts )
         : side{ s }
         , strategy{ std::move(ts) }
      {}

      ~Square() {}

      void translate( const Vector3D& v ) override { strategy( *this, v ); }

      double side;
      Vector3D center;
      TranslateStrategy strategy;
   };

   void translate( Square& square, const Vector3D& v )
   {
      square.center = square.center + v;
   }


   struct Translate {
      template< typename T >
      void operator()( T& t, const Vector3D& v ) const
      {
         translate( t, v );
      }
   };


   using Shapes = std::vector< std::unique_ptr<Shape> >;

   void translate( Shapes& shapes, const Vector3D& v )
   {
      for( auto& shape : shapes )
      {
         shape->translate( v );
      }
   }

} // namespace manual_function_solution


int main()
{
   const size_t N    ( 100UL );
   const size_t steps( 2500000UL );

   std::random_device rd{};
   const unsigned int seed( rd() );

   std::mt19937 rng{};
   std::uniform_real_distribution<double> dist( 0.0, 1.0 );

   {
      using namespace classic_solution;

      rng.seed( seed );

      Shapes shapes;

      for( size_t i=0UL; i<N; ++i ) {
         if( dist( rng ) < 0.5 )
            shapes.push_back( std::make_unique<Circle>( dist( rng )
                                                      , std::make_unique<ConcreteTranslateStrategy>() ) );
         else
            shapes.push_back( std::make_unique<Square>( dist( rng )
                                                      , std::make_unique<ConcreteTranslateStrategy>() ) );
      }

      std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
      start = std::chrono::high_resolution_clock::now();

      for( size_t s=0UL; s<steps; ++s ) {
         translate( shapes, Vector3D{ dist( rng ), dist( rng ) } );
      }

      end = std::chrono::high_resolution_clock::now();
      const std::chrono::duration<double> elapsedTime( end - start );
      const double seconds( elapsedTime.count() );

      std::cout << " Classic solution runtime         : " << seconds << "s\n";
   }

   {
      using namespace std_function_solution;

      rng.seed( seed );

      Shapes shapes;

      for( size_t i=0UL; i<N; ++i ) {
         if( dist( rng ) < 0.5 )
            shapes.push_back( std::make_unique<Circle>( dist( rng ), Translate{} ) );
         else
            shapes.push_back( std::make_unique<Square>( dist( rng ), Translate{} ) );
      }

      std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
      start = std::chrono::high_resolution_clock::now();

      for( size_t s=0UL; s<steps; ++s ) {
         translate( shapes, Vector3D{ dist( rng ), dist( rng ) } );
      }

      end = std::chrono::high_resolution_clock::now();
      const std::chrono::duration<double> elapsedTime( end - start );
      const double seconds( elapsedTime.count() );

      std::cout << " std::function solution runtime   : " << seconds << "s\n";
   }

   {
      using namespace manual_function_solution;

      rng.seed( seed );

      Shapes shapes;

      for( size_t i=0UL; i<N; ++i ) {
         if( dist( rng ) < 0.5 )
            shapes.push_back( std::make_unique<Circle>( dist( rng ), Translate{} ) );
         else
            shapes.push_back( std::make_unique<Square>( dist( rng ), Translate{} ) );
      }

      std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
      start = std::chrono::high_resolution_clock::now();

      for( size_t s=0UL; s<steps; ++s ) {
         translate( shapes, Vector3D{ dist( rng ), dist( rng ) } );
      }

      end = std::chrono::high_resolution_clock::now();
      const std::chrono::duration<double> elapsedTime( end - start );
      const double seconds( elapsedTime.count() );

      std::cout << " Manual function solution runtime : " << seconds << "s\n";
   }

   return EXIT_SUCCESS;
}

