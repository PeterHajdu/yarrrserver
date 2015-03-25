@smoke
Feature: Network access to model system
  To debug the system
  As a yarrr developer
  I should be able to access the model system on a running server process

  Scenario: starting the server with remote model port parameter
    When I start the server with a port and command line parameter --remote-model-endpoint tcp://*:32199
    Then the server should be running
    And I should be able to get model information from remote endpoint tcp://localhost:32199

