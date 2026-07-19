function(configure_gsxi_test TARGET_NAME TEST_NAME)
    target_link_libraries(${TARGET_NAME} PRIVATE Qt6::Core Qt6::Test)
    target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_SOURCE_DIR}")
    add_test(NAME ${TEST_NAME} COMMAND ${TARGET_NAME})

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:Qt6::Core>"
            "$<TARGET_FILE:Qt6::Test>"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>"
            VERBATIM)
endfunction()

function(gsxi_add_qt_test TARGET_NAME TEST_NAME)
    add_executable(${TARGET_NAME} ${ARGN})
    configure_gsxi_test(${TARGET_NAME} ${TEST_NAME})
endfunction()

add_library(gsxi-turnaround-state-test-support STATIC
        src/domain/model/AutomationStatus.h
        src/domain/model/AutomationSettings.h
        src/domain/turnaround/TurnaroundPhase.h
        src/domain/ports/Aircraft.h
        src/domain/ports/GsxGateway.h
        src/domain/ports/GsxMenuGateway.h
        src/domain/ports/DomainLogger.h
        ${TURNAROUND_STATE_SOURCES})
target_include_directories(gsxi-turnaround-state-test-support PRIVATE "${CMAKE_SOURCE_DIR}")

function(add_turnaround_state_test TARGET_NAME TEST_NAME TEST_FILE)
    add_executable(${TARGET_NAME}
            tests/turnaround/TurnaroundStateFixture.h
            tests/doubles/FakeAircraft.h
            tests/doubles/FakeDomainLogger.h
            tests/doubles/FakeGsxMenuGateway.h
            tests/doubles/FakeGsxService.h
            ${TEST_FILE})
    target_link_libraries(${TARGET_NAME} PRIVATE gsxi-turnaround-state-test-support)
    configure_gsxi_test(${TARGET_NAME} ${TEST_NAME})
endfunction()

set(TURNAROUND_STATE_TESTS
        gsxi-turnaround-waiting-supported-aircraft-state-tests turnaround-state-waiting-supported-aircraft tests/turnaround/states/tst_waiting_supported_aircraft_state.cpp
        gsxi-turnaround-waiting-flight-plan-state-tests turnaround-state-waiting-flight-plan tests/turnaround/states/tst_waiting_flight_plan_state.cpp
        gsxi-turnaround-reposition-aircraft-state-tests turnaround-state-reposition-aircraft tests/turnaround/states/tst_reposition_aircraft_state.cpp
        gsxi-turnaround-place-ground-equipment-state-tests turnaround-state-place-ground-equipment tests/turnaround/states/tst_place_ground_equipment_state.cpp
        gsxi-turnaround-call-services-state-tests turnaround-state-call-services tests/turnaround/states/tst_call_services_state.cpp
        gsxi-turnaround-waiting-power-on-state-tests turnaround-state-waiting-power-on tests/turnaround/states/tst_waiting_power_on_state.cpp
        gsxi-turnaround-request-fuel-state-tests turnaround-state-request-fuel tests/turnaround/states/tst_request_fuel_state.cpp
        gsxi-turnaround-refueling-state-tests turnaround-state-refueling tests/turnaround/states/tst_refueling_state.cpp
        gsxi-turnaround-request-boarding-state-tests turnaround-state-request-boarding tests/turnaround/states/tst_request_boarding_state.cpp
        gsxi-turnaround-boarding-state-tests turnaround-state-boarding tests/turnaround/states/tst_boarding_state.cpp
        gsxi-turnaround-waiting-aircraft-ready-state-tests turnaround-state-waiting-aircraft-ready tests/turnaround/states/tst_waiting_aircraft_ready_state.cpp
        gsxi-turnaround-waiting-ready-to-push-state-tests turnaround-state-waiting-ready-to-push tests/turnaround/states/tst_waiting_ready_to_push_state.cpp
        gsxi-turnaround-wait-catering-state-tests turnaround-state-wait-catering tests/turnaround/states/tst_wait_catering_state.cpp
        gsxi-turnaround-remove-ground-equipment-state-tests turnaround-state-remove-ground-equipment tests/turnaround/states/tst_remove_ground_equipment_state.cpp
        gsxi-turnaround-request-pushback-state-tests turnaround-state-request-pushback tests/turnaround/states/tst_request_pushback_state.cpp
        gsxi-turnaround-waiting-engines-state-tests turnaround-state-waiting-engines tests/turnaround/states/tst_waiting_engines_state.cpp
        gsxi-turnaround-waiting-pushback-to-start-state-tests turnaround-state-waiting-pushback-to-start tests/turnaround/states/tst_waiting_pushback_to_start_state.cpp
        gsxi-turnaround-waiting-departure-state-tests turnaround-state-waiting-departure tests/turnaround/states/tst_waiting_departure_state.cpp
        gsxi-turnaround-on-flight-state-tests turnaround-state-on-flight tests/turnaround/states/tst_on_flight_state.cpp
        gsxi-turnaround-place-arrival-ground-equipment-state-tests turnaround-state-place-arrival-ground-equipment tests/turnaround/states/tst_place_arrival_ground_equipment_state.cpp
        gsxi-turnaround-waiting-engine-shutdown-state-tests turnaround-state-waiting-engine-shutdown tests/turnaround/states/tst_waiting_engine_shutdown_state.cpp
        gsxi-turnaround-request-deboarding-state-tests turnaround-state-request-deboarding tests/turnaround/states/tst_request_deboarding_state.cpp
        gsxi-turnaround-deboarding-state-tests turnaround-state-deboarding tests/turnaround/states/tst_deboarding_state.cpp
        gsxi-turnaround-cabin-services-state-tests turnaround-state-cabin-services tests/turnaround/states/tst_cabin_services_state.cpp
        gsxi-turnaround-waiting-new-flight-state-tests turnaround-state-waiting-new-flight tests/turnaround/states/tst_waiting_new_flight_state.cpp
)

list(LENGTH TURNAROUND_STATE_TESTS TURNAROUND_STATE_TEST_COUNT)
math(EXPR TURNAROUND_STATE_TEST_LAST_INDEX "${TURNAROUND_STATE_TEST_COUNT} - 1")
foreach (TURNAROUND_STATE_TEST_INDEX RANGE 0 ${TURNAROUND_STATE_TEST_LAST_INDEX} 3)
    math(EXPR TURNAROUND_STATE_TEST_NAME_INDEX "${TURNAROUND_STATE_TEST_INDEX} + 1")
    math(EXPR TURNAROUND_STATE_TEST_FILE_INDEX "${TURNAROUND_STATE_TEST_INDEX} + 2")
    list(GET TURNAROUND_STATE_TESTS ${TURNAROUND_STATE_TEST_INDEX} TURNAROUND_STATE_TARGET)
    list(GET TURNAROUND_STATE_TESTS ${TURNAROUND_STATE_TEST_NAME_INDEX} TURNAROUND_STATE_TEST_NAME)
    list(GET TURNAROUND_STATE_TESTS ${TURNAROUND_STATE_TEST_FILE_INDEX} TURNAROUND_STATE_TEST_FILE)
    add_turnaround_state_test(${TURNAROUND_STATE_TARGET}
            ${TURNAROUND_STATE_TEST_NAME}
            ${TURNAROUND_STATE_TEST_FILE})
endforeach ()

add_executable(gsxi-turnaround-workflow-tests
        tests/TestDoubles.h
        tests/turnaround/TurnaroundStateFixture.h
        tests/doubles/FakeGsxMenuGateway.h
        tests/doubles/FakeDomainLogger.h
        tests/tst_state_machine.cpp
        src/domain/turnaround/TurnaroundStateMachine.cpp
        src/domain/turnaround/TurnaroundStateMachine.h)
target_link_libraries(gsxi-turnaround-workflow-tests PRIVATE gsxi-turnaround-state-test-support)
configure_gsxi_test(gsxi-turnaround-workflow-tests turnaround-workflow)

gsxi_add_qt_test(gsxi-operations-viewmodel-tests operations-viewmodel
        tests/TestDoubles.h
        tests/tst_operations_viewmodel.cpp
        src/viewmodel/OperationsViewModel.cpp
        src/viewmodel/OperationsViewModel.h)

gsxi_add_qt_test(gsxi-settings-viewmodel-tests settings-viewmodel
        tests/TestDoubles.h
        tests/tst_settings_viewmodel.cpp
        src/viewmodel/SettingsViewModel.cpp
        src/viewmodel/SettingsViewModel.h)

gsxi_add_qt_test(gsxi-simbrief-ofp-tests simbrief-ofp
        tests/tst_simbrief_ofp.cpp
        src/domain/model/FlightPlan.h
        src/infrastructure/simbrief/SimbriefOfpParser.cpp
        src/infrastructure/simbrief/SimbriefOfpParser.h
        src/domain/support/Weight.h)

gsxi_add_qt_test(gsxi-weight-tests weight
        tests/tst_weight.cpp
        src/domain/support/Weight.h)

gsxi_add_qt_test(gsxi-session-readiness-tests session-readiness
        tests/tst_session_readiness.cpp
        src/application/sim/SessionReadiness.h
        src/application/sim/SimVersion.h)

gsxi_add_qt_test(gsxi-turnaround-math-tests turnaround-math
        tests/tst_turnaround_math.cpp
        src/domain/turnaround/TurnaroundMath.h)

gsxi_add_qt_test(gsxi-automation-status-tests automation-status
        tests/tst_automation_status.cpp
        src/domain/model/AutomationStatus.h
        src/domain/model/FlightPlan.h)

gsxi_add_qt_test(gsxi-gsx-plugin-client-tests gsx-plugin-client
        tests/TestDoubles.h
        tests/tst_gsx_plugin_client.cpp
        src/infrastructure/commbus/CommBusPluginClient.cpp
        src/infrastructure/commbus/CommBusPluginClient.h
        src/infrastructure/simvars/VariableGateway.h)

gsxi_add_qt_test(gsxi-remote-state-tests remote-state
        tests/tst_remote_state.cpp
        src/infrastructure/gsx/GsxRemoteState.h
        src/infrastructure/gsx/GsxRemoteStateReducer.cpp
        src/infrastructure/gsx/GsxRemoteStateReducer.h)
target_compile_definitions(gsxi-remote-state-tests PRIVATE
        GSX_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\")

gsxi_add_qt_test(gsxi-gsx-menu-navigator-tests gsx-menu-navigator
        tests/doubles/FakeDomainLogger.h
        tests/doubles/FakeVariableGateway.h
        tests/tst_gsx_menu_navigator.cpp
        src/infrastructure/gsx/GsxMenuNavigator.cpp
        src/infrastructure/gsx/GsxMenuNavigator.h
        src/infrastructure/gsx/GsxRemoteApiClient.cpp
        src/infrastructure/gsx/GsxRemoteApiClient.h
        src/infrastructure/gsx/GsxRemoteState.h
        src/infrastructure/commbus/CommBusPluginClient.cpp
        src/infrastructure/commbus/CommBusPluginClient.h
        src/infrastructure/simvars/VariableGateway.h
        src/domain/ports/GsxMenuGateway.h
        src/domain/ports/DomainLogger.h
        src/domain/model/AutomationSettings.h)
target_link_libraries(gsxi-gsx-menu-navigator-tests PRIVATE Qt6::WebSockets)

add_custom_command(TARGET gsxi-gsx-menu-navigator-tests POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:Qt6::WebSockets>"
        "$<TARGET_FILE:Qt6::Network>"
        "$<TARGET_FILE_DIR:gsxi-gsx-menu-navigator-tests>"
        VERBATIM)

gsxi_add_qt_test(gsxi-gsx-interface-tests gsx-interface
        tests/TestDoubles.h
        tests/tst_gsx_interface.cpp
        src/infrastructure/gsx/GsxStateService.cpp
        src/infrastructure/gsx/GsxStateService.h
        src/infrastructure/gsx/GsxRemoteState.h
        src/infrastructure/commbus/CommBusPluginClient.cpp
        src/infrastructure/commbus/CommBusPluginClient.h
        src/infrastructure/simvars/VariableGateway.h)

gsxi_add_qt_test(gsxi-gsx-remote-api-client-tests gsx-remote-api-client
        tests/tst_gsx_remote_api_client.cpp
        src/infrastructure/gsx/GsxRemoteApiClient.cpp
        src/infrastructure/gsx/GsxRemoteApiClient.h)
target_link_libraries(gsxi-gsx-remote-api-client-tests PRIVATE Qt6::WebSockets)

add_custom_command(TARGET gsxi-gsx-remote-api-client-tests POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:Qt6::WebSockets>"
        "$<TARGET_FILE:Qt6::Network>"
        "$<TARGET_FILE_DIR:gsxi-gsx-remote-api-client-tests>"
        VERBATIM)

gsxi_add_qt_test(gsxi-github-update-service-tests github-update-service
        tests/tst_github_update_service.cpp
        src/infrastructure/update/GithubUpdateService.cpp
        src/infrastructure/update/GithubUpdateService.h
        src/infrastructure/update/GithubReleaseParser.cpp
        src/infrastructure/update/GithubReleaseParser.h
        src/infrastructure/update/CommbusInstallProbe.cpp
        src/infrastructure/update/CommbusInstallProbe.h
        src/application/ports/UpdateService.h
        src/application/model/UpdateInfo.h)
target_link_libraries(gsxi-github-update-service-tests PRIVATE Qt6::Network)

add_custom_command(TARGET gsxi-github-update-service-tests POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:Qt6::Network>"
        "$<TARGET_FILE_DIR:gsxi-github-update-service-tests>"
        VERBATIM)

gsxi_add_qt_test(gsxi-qsettings-repository-tests qsettings-repository
        tests/tst_qsettings_repository.cpp
        src/infrastructure/settings/QSettingsRepository.cpp
        src/infrastructure/settings/QSettingsRepository.h
        src/application/ports/SettingsRepository.h
        src/application/model/AppSettings.h
        src/application/model/AircraftProfile.h)

gsxi_add_qt_test(gsxi-simconnect-session-tests simconnect-session
        tests/doubles/FakeSimConnectApi.h
        tests/doubles/FakeSimConnectApi.cpp
        tests/tst_simconnect_session.cpp
        src/infrastructure/simconnect/SimConnectSession.cpp
        src/infrastructure/simconnect/SimConnectSession.h
        src/infrastructure/simconnect/SimConnectVariableGateway.cpp
        src/infrastructure/simconnect/SimConnectVariableGateway.h)
target_include_directories(gsxi-simconnect-session-tests PRIVATE "${SIMCONNECT_INCLUDE_DIR}")

gsxi_add_qt_test(gsxi-variable-gateway-tests variable-gateway
        tests/tst_variable_gateway.cpp
        src/infrastructure/simconnect/SimConnectVariableGateway.cpp
        src/infrastructure/simconnect/SimConnectVariableGateway.h)
target_include_directories(gsxi-variable-gateway-tests PRIVATE "${SIMCONNECT_INCLUDE_DIR}")
target_link_libraries(gsxi-variable-gateway-tests PRIVATE "${SIMCONNECT_IMPORT_LIB}")
if (EXISTS "${SIMCONNECT_DLL}")
    add_custom_command(TARGET gsxi-variable-gateway-tests POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${SIMCONNECT_DLL}"
            "$<TARGET_FILE_DIR:gsxi-variable-gateway-tests>"
            VERBATIM)
endif ()

gsxi_add_qt_test(gsxi-tfdi-md11-tests tfdi-md11
        tests/TestDoubles.h
        tests/tst_tfdi_md11.cpp
        src/infrastructure/aircraft/AircraftIdentity.h
        src/infrastructure/aircraft/AircraftRegistry.cpp
        src/infrastructure/aircraft/AircraftRegistry.h
        src/infrastructure/aircraft/TfdiMd11.cpp
        src/infrastructure/aircraft/TfdiMd11.h
        src/domain/model/AutomationStatus.h
        src/domain/support/Weight.h)

gsxi_add_qt_test(gsxi-ifly-737max-tests ifly-737max
        tests/TestDoubles.h
        tests/tst_ifly_737max.cpp
        src/infrastructure/aircraft/AircraftIdentity.h
        src/infrastructure/aircraft/AircraftRegistry.cpp
        src/infrastructure/aircraft/AircraftRegistry.h
        src/infrastructure/aircraft/IFly737Max.cpp
        src/infrastructure/aircraft/IFly737Max.h
        src/domain/model/AutomationStatus.h)

gsxi_add_qt_test(gsxi-gsx-aircraft-profile-tests gsx-aircraft-profile
        tests/tst_gsx_aircraft_profile.cpp
        src/infrastructure/gsx/GsxAircraftProfile.cpp
        src/infrastructure/gsx/GsxAircraftProfile.h)

gsxi_add_qt_test(gsxi-toliss-a340-tests toliss-a340
        tests/TestDoubles.h
        tests/tst_toliss_a340.cpp
        src/infrastructure/aircraft/AircraftIdentity.h
        src/infrastructure/aircraft/AircraftRegistry.cpp
        src/infrastructure/aircraft/AircraftRegistry.h
        src/infrastructure/aircraft/TolissA340.cpp
        src/infrastructure/aircraft/TolissA340.h
        src/domain/model/AutomationStatus.h)

gsxi_add_qt_test(gsxi-aircraft-matching-tests aircraft-matching
        tests/tst_aircraft_matching.cpp
        src/infrastructure/aircraft/AircraftIdentity.h
        src/infrastructure/aircraft/AircraftRegistry.cpp
        src/infrastructure/aircraft/AircraftRegistry.h)

gsxi_add_qt_test(gsxi-aircraft-detection-tests aircraft-detection
        tests/TestDoubles.h
        tests/tst_aircraft_detection.cpp
        src/infrastructure/aircraft/AircraftFactory.cpp
        src/infrastructure/aircraft/AircraftFactory.h
        src/infrastructure/aircraft/AircraftIdentity.h
        src/infrastructure/aircraft/AircraftRegistry.cpp
        src/infrastructure/aircraft/AircraftRegistry.h
        src/infrastructure/aircraft/IFly737Max.cpp
        src/infrastructure/aircraft/IFly737Max.h
        src/infrastructure/aircraft/TfdiMd11.cpp
        src/infrastructure/aircraft/TfdiMd11.h
        src/infrastructure/aircraft/TolissA340.cpp
        src/infrastructure/aircraft/TolissA340.h
        src/domain/model/AutomationStatus.h
        src/domain/support/Weight.h)

gsxi_add_qt_test(gsxi-github-release-parser-tests github-release-parser
        tests/tst_github_release_parser.cpp
        src/application/model/UpdateInfo.h
        src/infrastructure/update/GithubReleaseParser.cpp
        src/infrastructure/update/GithubReleaseParser.h)
target_compile_definitions(gsxi-github-release-parser-tests PRIVATE
        GSX_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\")

gsxi_add_qt_test(gsxi-update-viewmodel-tests update-viewmodel
        tests/doubles/FakeUpdateService.h
        tests/tst_update_viewmodel.cpp
        src/application/model/UpdateInfo.h
        src/application/ports/UpdateService.h
        src/viewmodel/UpdateViewModel.cpp
        src/viewmodel/UpdateViewModel.h)

gsxi_add_qt_test(gsxi-commbus-install-probe-tests commbus-install-probe
        tests/tst_commbus_install_probe.cpp
        src/infrastructure/update/CommbusInstallProbe.cpp
        src/infrastructure/update/CommbusInstallProbe.h)

gsxi_add_qt_test(gsxi-automation-settings-tests automation-settings
        tests/tst_automation_settings.cpp
        src/domain/model/AutomationSettings.h
        src/application/model/AircraftProfile.h
        src/application/model/EffectiveSettings.h)

gsxi_add_qt_test(gsxi-command-result-tests command-result
        tests/tst_command_result.cpp
        src/application/model/CommandResult.h)

gsxi_add_qt_test(gsxi-integrator-snapshot-tests integrator-snapshot
        tests/tst_integrator_snapshot.cpp
        src/application/model/IntegratorSnapshot.h
        src/domain/model/FlightPlan.h
        src/domain/turnaround/TurnaroundPhase.h)

gsxi_add_qt_test(gsxi-runtime-integrator-service-tests runtime-integrator-service
        tests/doubles/FakeSimConnectApi.h
        tests/doubles/FakeSimConnectApi.cpp
        tests/tst_runtime_integrator_service.cpp
        src/application/IntegratorRuntime.cpp
        src/application/IntegratorRuntime.h
        src/application/RuntimeIntegratorService.cpp
        src/application/RuntimeIntegratorService.h
        src/domain/turnaround/TurnaroundStateMachine.cpp
        src/domain/turnaround/TurnaroundStateMachine.h
        src/infrastructure/aircraft/AircraftFactory.cpp
        src/infrastructure/aircraft/AircraftFactory.h
        src/infrastructure/aircraft/AircraftRegistry.cpp
        src/infrastructure/aircraft/AircraftRegistry.h
        src/infrastructure/aircraft/IFly737Max.cpp
        src/infrastructure/aircraft/IFly737Max.h
        src/infrastructure/aircraft/TfdiMd11.cpp
        src/infrastructure/aircraft/TfdiMd11.h
        src/infrastructure/aircraft/TolissA340.cpp
        src/infrastructure/aircraft/TolissA340.h
        src/infrastructure/commbus/CommBusPluginClient.cpp
        src/infrastructure/commbus/CommBusPluginClient.h
        src/infrastructure/gsx/GsxAircraftProfile.cpp
        src/infrastructure/gsx/GsxAircraftProfile.h
        src/infrastructure/gsx/GsxMenuNavigator.cpp
        src/infrastructure/gsx/GsxMenuNavigator.h
        src/infrastructure/gsx/GsxRemoteApiClient.cpp
        src/infrastructure/gsx/GsxRemoteApiClient.h
        src/infrastructure/gsx/GsxRemoteStateReducer.cpp
        src/infrastructure/gsx/GsxRemoteStateReducer.h
        src/infrastructure/gsx/GsxStateService.cpp
        src/infrastructure/gsx/GsxStateService.h
        src/infrastructure/logging/QtDomainLogger.cpp
        src/infrastructure/logging/QtDomainLogger.h
        src/infrastructure/simbrief/SimbriefClient.cpp
        src/infrastructure/simbrief/SimbriefClient.h
        src/infrastructure/simbrief/SimbriefOfpParser.cpp
        src/infrastructure/simbrief/SimbriefOfpParser.h
        src/infrastructure/simconnect/SimConnectSession.cpp
        src/infrastructure/simconnect/SimConnectSession.h
        src/infrastructure/simconnect/SimConnectVariableGateway.cpp
        src/infrastructure/simconnect/SimConnectVariableGateway.h)
target_link_libraries(gsxi-runtime-integrator-service-tests PRIVATE
        gsxi-turnaround-state-test-support
        Qt6::Network
        Qt6::WebSockets)
target_include_directories(gsxi-runtime-integrator-service-tests PRIVATE "${SIMCONNECT_INCLUDE_DIR}")

add_custom_command(TARGET gsxi-runtime-integrator-service-tests POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:Qt6::WebSockets>"
        "$<TARGET_FILE:Qt6::Network>"
        "$<TARGET_FILE_DIR:gsxi-runtime-integrator-service-tests>"
        VERBATIM)
