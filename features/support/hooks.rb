Before do
  @notification_file_path = "./notifications"
  @notification_file = File.open( @notification_file_path, "w+" )
end

After do
  @yarrr_server.kill
  @yarrr_client.kill if not @yarrr_client.nil?
end

