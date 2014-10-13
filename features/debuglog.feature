
@smoke
Feature: Specify loglevel with command line parameter
  To be able to investigate problems with much more detail
  As a yarrr developer
  I should be able to specify the loglevel

  Scenario: specify with command line parameter
    When I start the server with a port and command line parameter --loglevel 0
    Then the server should be running
    And I should see Loglevel is set to 0

  Scenario: default loglevel
    Given a running server
    Then I should see Loglevel is set to 100

