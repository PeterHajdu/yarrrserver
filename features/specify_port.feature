@smoke
Feature: Specify port number from the command line
  To be able to start many server instances on the same host
  As a yarrr developer
  I should be able to specify the server port

  Scenario: specify with command line parameter
    When I start the server with command line parameter --port 21345
    Then the server should be running
    And I should be able to connect to port 21345

  Scenario: prints out usage when starting without the port parameter
    When I start the server without any command line parameter
    Then the server should not be running
    And It should print the usage text to the terminal

