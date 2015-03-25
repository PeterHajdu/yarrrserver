Before do
  @notification_file_path = "./notifications"
  @notification_file = File.open( @notification_file_path, "w+" )
end

After do
  @yarrr_server.kill
  @yarrr_client.kill if not @yarrr_client.nil?

  if not @yarrr_clients.nil? then

    @yarrr_clients.each do | client |
      client.kill
    end

  end

end

