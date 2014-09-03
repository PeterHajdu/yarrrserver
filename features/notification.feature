@smoke
Feature: Send notification on various events
  To be able to react fast to events
  As a yarrr developer
  I should be able to get notification from the server

  Scenario: send notification if new player logs in
    Given a running server
    When I start a client
    Then I should receive a notification

