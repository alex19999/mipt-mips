

// Google Test Library
#include <gtest/gtest.h>


// Module
#include "../ports.h"


#include <cassert>
#include <map>


namespace ports {
    namespace testing {

        static const Latency PORT_LATENCY = 1_Lt;
        static const uint32 PORT_FANOUT = 1;
        static const uint32 PORT_BANDWIDTH = 1;

        static const int DATA_LIMIT = 5;
        static const Cycle CLOCK_LIMIT = 10_Cl;

        class Logic
        {
            public:
                enum class CheckCode
                {
                    DATA_FROM_A,
                    DATA_FROM_B,
                    DATA_INIT,
                    IS_FROM_A_READY,
                    IS_FROM_B_READY,
                    IS_INIT_READY,
                    IS_STOP_READY
                };

                bool check_readiness( Cycle cycle, CheckCode code, bool is_ready);
                bool check_data( Cycle cycle, CheckCode code, int data);

            private:
                static const int NONE = -1;
                static const Cycle EXPECTED_MAX_CYCLE;

                struct State
                {
                    bool is_from_A_ready;
                    bool is_from_B_ready;
                    bool is_init_ready;
                    int data_from_A;
                    int data_from_B;
                };

                std::map<Cycle, State> blogic=
                //  | CYCLE |IS_FROM_A_READY|IS_FROM_B_READY|IS_INIT_READY|DATA_FROM_A|DATA_FROM_B|
                {
                   { 0_Cl, { false,          false,          false,        NONE,       NONE        }},
                   { 1_Cl, { false,          false,          true,         NONE,       NONE        }},
                   { 2_Cl, { true,           false,          false,        1,          NONE        }},
                   { 3_Cl, { false,          true,           false,        NONE,       2           }},
                   { 4_Cl, { true,           false,          false,        3,          NONE        }},
                   { 5_Cl, { false,          true,           false,        NONE,       4           }},
                   { 6_Cl, { true,           false,          false,        5,          NONE        }},
                   { 7_Cl, { false,          true,           false,        NONE,       6           }},
                   { 8_Cl, { false,          false,          false,        NONE,       NONE        }}
                };
        };

        const Cycle Logic::EXPECTED_MAX_CYCLE = 8_Cl;


        class A
        {
            public:
                A();
                void clock( Cycle cycle);

            private:
                std::unique_ptr<WritePort<int>> to_B;
                std::unique_ptr<ReadPort<int>> from_B;

                std::unique_ptr<ReadPort<int>> init;
                std::unique_ptr<WritePort<bool>> stop;

                void process_data( int data, Cycle cycle);
        };



        class B
        {
            public:
                B();
                void clock( Cycle cycle);

            private:
                std::unique_ptr<WritePort<int>> to_A;
                std::unique_ptr<ReadPort<int>> from_A;

                void process_data ( int data, Cycle cycle);
        };




        static Logic logic{};
    } // namespace testing
} // namespace ports







#define GTEST_ASSERT_NO_DEATH( statement)                        \
    ASSERT_EXIT( { statement}::exit(EXIT_SUCCESS),               \
                 ::testing::ExitedWithCode(EXIT_SUCCESS), "");   \
    statement




using ports::testing::Logic;




TEST( test_ports, Test_Ports_A_B)
{
    GTEST_ASSERT_NO_DEATH( ports::testing::A a;);
    GTEST_ASSERT_NO_DEATH( ports::testing::B b;);


    GTEST_ASSERT_NO_DEATH( WritePort<int> init( "init_A", ports::testing::PORT_BANDWIDTH,
                                                ports::testing::PORT_FANOUT););
    GTEST_ASSERT_NO_DEATH( ReadPort<bool> stop( "stop", ports::testing::PORT_LATENCY););


    // connect all the ports
    GTEST_ASSERT_NO_DEATH( init_ports(););

    // init object A by value 0
    GTEST_ASSERT_NO_DEATH( init.write( 0, 0_Cl););


    for ( auto cycle = 0_Cl; cycle < ports::testing::CLOCK_LIMIT; cycle.inc())
    {
        //check the stop port from object A
        bool is_ready;
        GTEST_ASSERT_NO_DEATH( is_ready = stop.is_ready( cycle););
        ASSERT_TRUE( ports::testing::logic.check_readiness( cycle,
                                                            Logic::CheckCode::IS_STOP_READY,
                                                            is_ready));
        if ( is_ready)
        {
            GTEST_ASSERT_NO_DEATH( stop.read( cycle););
            break;
        }

        a.clock( cycle);
        b.clock( cycle);

        GTEST_ASSERT_NO_DEATH( check_ports( cycle););
    }

    GTEST_ASSERT_NO_DEATH( destroy_ports(););
}




int main( int argc, char** argv)
{
    ::testing::InitGoogleTest( &argc, argv);
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    return RUN_ALL_TESTS();
}




namespace ports {
    namespace testing {

        bool Logic::check_readiness( Cycle cycle, CheckCode code, bool is_ready)
        {
            if ( cycle > EXPECTED_MAX_CYCLE)
                return false;

            bool is_ok = false;
            switch ( code)
            {
                case CheckCode::IS_STOP_READY:
                    // STOP port should be ready only in the expected max cycle
                    return (cycle == EXPECTED_MAX_CYCLE) == is_ready;

                case CheckCode::IS_INIT_READY:
                    is_ok = blogic[cycle].is_init_ready == is_ready;

                    // If it is ready it will be read in the current cycle
                    // so blogic should be updated to not interfere with
                    // subsequent checks in the same cycle
                    if ( is_ok && is_ready)
                        blogic[cycle].is_init_ready = false;

                    break;

                case CheckCode::IS_FROM_A_READY:
                    is_ok = blogic[cycle].is_from_A_ready == is_ready;

                    // If it is ready it will be read in the current cycle
                    // so blogic should be updated to not interfere with
                    // subsequent checks in the same cycle
                    if ( is_ok && is_ready)
                        blogic[cycle].is_from_A_ready = false;

                    break;

                case CheckCode::IS_FROM_B_READY:
                    is_ok = blogic[cycle].is_from_B_ready == is_ready;

                    // If it is ready it will be read in the current cycle
                    // so blogic should be updated to not interfere with
                    // subsequent checks in the same cycle
                    if ( is_ok && is_ready)
                        blogic[cycle].is_from_B_ready = false;

                    break;

                default:
                    break;
            }

            return is_ok;
        }

        bool Logic::check_data( Cycle cycle, CheckCode code, int data)
        {
            if ( cycle > EXPECTED_MAX_CYCLE || data < 0)
                return false;

            switch ( code)
            {
                case CheckCode::DATA_INIT:
                    // data from init port should be equal to 0
                    // merely in cycle 1
                    return cycle == 1_Cl && data == 0;

                case CheckCode::DATA_FROM_A:
                    return blogic[cycle].data_from_A == data;

                case CheckCode::DATA_FROM_B:
                    return blogic[cycle].data_from_B == data;

                default:
                    return false;
            };
        }



        A::A()
            : to_B{ make_write_port<int>( "A_to_B", PORT_BANDWIDTH, PORT_FANOUT)}
            , from_B{ make_read_port<int>( "B_to_A", PORT_LATENCY)}

            , init{ make_read_port<int> ( "init_A", PORT_LATENCY)}
            , stop{ make_write_port<bool> ( "stop", PORT_BANDWIDTH, PORT_FANOUT)}
        { }

        void A::process_data( int data, Cycle cycle)
        {
            ++data;

            // If data limit is exceeded
            // stop signal will be sent
            if ( data > DATA_LIMIT)
            {
                GTEST_ASSERT_NO_DEATH( stop->write( true, cycle););
                return;
            }

            GTEST_ASSERT_NO_DEATH( to_B->write( data, cycle););
        }

        void A::clock( Cycle cycle)
        {
            int data;

            bool is_init_ready;
            bool is_from_B_ready;
            while( true)
            {
                GTEST_ASSERT_NO_DEATH( is_init_ready = init->is_ready( cycle););
                ASSERT_TRUE( ports::testing::logic.check_readiness( cycle,
                                                                    Logic::CheckCode::IS_INIT_READY,
                                                                    is_init_ready));

                GTEST_ASSERT_NO_DEATH( is_from_B_ready = from_B->is_ready( cycle););
                ASSERT_TRUE( ports::testing::logic.check_readiness( cycle,
                                                                    Logic::CheckCode::IS_FROM_B_READY,
                                                                    is_from_B_ready));

                if( is_init_ready)
                {
                    GTEST_ASSERT_NO_DEATH( data = init->read( cycle););
                    ASSERT_TRUE( ports::testing::logic.check_data( cycle,
                                                                   Logic::CheckCode::DATA_INIT,
                                                                   data));
                }
                else if ( is_from_B_ready)
                {
                    GTEST_ASSERT_NO_DEATH( data = from_B->read( cycle););
                    ASSERT_TRUE( ports::testing::logic.check_data( cycle,
                                                                   Logic::CheckCode::DATA_FROM_B,
                                                                   data));
                }
                else
                    break;

                process_data( data, cycle);
            }
        }



        B::B()
            : to_A{ make_write_port<int>( "B_to_A", PORT_BANDWIDTH, PORT_FANOUT)}
            , from_A{ make_read_port<int>( "A_to_B", PORT_LATENCY)}
        { }

        void B::process_data( int data, Cycle cycle)
        {
            ++data;

            GTEST_ASSERT_NO_DEATH( to_A->write( data, cycle););
        }

        void B::clock( Cycle cycle)
        {
            bool is_from_A_ready;
            GTEST_ASSERT_NO_DEATH( is_from_A_ready = from_A->is_ready( cycle););
            ASSERT_TRUE( ports::testing::logic.check_readiness( cycle,
                                                                Logic::CheckCode::IS_FROM_A_READY,
                                                                is_from_A_ready));
            if ( is_from_A_ready)
            {
                int data;
                GTEST_ASSERT_NO_DEATH( data = from_A->read( cycle););
                ASSERT_TRUE( ports::testing::logic.check_data( cycle,
                                                               Logic::CheckCode::DATA_FROM_A,
                                                               data));

                GTEST_ASSERT_NO_DEATH( process_data( data, cycle););
            }
        }

    } // namespace testing
} // namespace ports

