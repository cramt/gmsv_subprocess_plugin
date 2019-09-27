require("nfgs_plugin")

function initNode(path)
    local returnObj = {
        listeners = {},
        ptr = ___NFGS___WRAPPER___TABLE___.instantiateNodeEnv(path),
        running = true,
    }
    hook.Add("Tick", returnObj.ptr, function()
        if(returnObj.running) then
            returnObj:think()
        end
    end)
    function returnObj:addListener(key, func)
        self.listeners[key] = self.listeners[key] or {}
        table.insert(self.listeners[key], func)
    end
    function returnObj:removeListener(key, func)
        table.remove(self.listeners[key], func)
    end
    function returnObj:think()
        local nodeCommands = ___NFGS___WRAPPER___TABLE___.think(self.ptr);
        if(nodeCommands == nil) then
            return nil;
        end
        if(nodeCommands == false) then
            self.running = false;
            return false;
        end
        for _, v in pairs(nodeCommands) do
            local command = util.JSONToTable(v)
            if(command.command == "event" or command.command == "exception") then
                local listeners = self.listeners[command.eventName] or {}
                for _, f in pairs(listeners) do
                    f(command.data)
                end
            end
            if(command.command == "print") then
                print(command.data)
            end
            if(command.command == "exception") then
                print(command.eventName + " happened: " + command.data)
                if(command.eventName == "uncaughtException") then
                    self:kill();
                end
            end
        end
        return true;
    end
    function returnObj:kill()
        running = false;
        hook.Remove("Tick", returnObj.ptr)
        ___NFGS___WRAPPER___TABLE___.kill(self.ptr)
    end
    function returnObj:send(key, data)
        ___NFGS___WRAPPER___TABLE___.send(self.ptr, util.TableToJSON({
            eventName = key,
            data = data
        }))
    end
    return returnObj;
end


