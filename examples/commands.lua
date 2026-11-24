registerCommand("pay", function(playerId, params)
    local ok, target, amount = sscanf(params, "ud")
    if not ok then
        return sendMessage(playerId, 0xFFC107FF, "Usage: /pay [player] [amount]")
    end
    if not target or target == playerId then
        return sendMessage(playerId, 0xFF4D4DFF, "Player not found.")
    end
    if amount < 1 or getPlayerMoney(playerId) < amount then
        return sendMessage(playerId, 0xFF4D4DFF, "Invalid amount.")
    end
    givePlayerMoney(playerId, -amount)
    givePlayerMoney(target, amount)
end)

registerCommand("pm", function(playerId, params)
    local ok, target, text = sscanf(params, "us")
    if not ok then
        return sendMessage(playerId, 0xFFC107FF, "Usage: /pm [player] [message]")
    end
    if not target then
        return sendMessage(playerId, 0xFF4D4DFF, "Player not found.")
    end
    sendMessage(target, 0xFACC15FF, getPlayerName(playerId) .. ": " .. text)
end)

registerCommand("veh", function(playerId, params)
    local ok, model, color1, color2 = sscanf(params, "dD(1)D(1)")
    if not ok then
        return sendMessage(playerId, 0xFFC107FF, "Usage: /veh [model] [color1?] [color2?]")
    end
    local x, y, z = getPlayerCoordinates(playerId)
    spawnVehicle(model, x + 3, y, z, 0, color1, color2, -1, false)
end)

registerCommand("tpto", function(playerId, params)
    local ok, x, y, z = sscanf(params, "p<,>fff")
    if not ok then
        return sendMessage(playerId, 0xFFC107FF, "Usage: /tpto x, y, z")
    end
    setPlayerCoordinates(playerId, x, y, z)
end)
