require("nfgs_plugin")

function initNode()
    local returnObj = {
        listeners = {},
        ptr = ___NFGS___WRAPPER___TABLE___.instantiateNodeEnv(),
        running = true,
    }
    function returnObj:addListener(key, func)
        self.listeners[key] = self.listeners[key] or {}
        table.insert(self.listeners[key], func)
    end
    function returnObj:removeListener(key, func)
        table.remove(self.listeners[key], func)
    end
    function returnObj:think()
        local nodeCommands = ___NFGS___WRAPPER___TABLE___.think(self.ptr);
        if(nodeCommands == false){
            self.running = false;
            return false;
        }
        for _, v in pairs(nodeCommands) do
            local command = util.JSONToTable(v)
            if(command.command == "event" or command.command == "exception") do
                local listeners = self.listeners[command.eventName] or {}
                for _, f in pairs(listeners) do
                    f(comamnd.data)
                end
            end
            if(command.command == "print") do
                print(command.data)
            end
            if(command.command == "exception") do
                print(command.eventName + " happened: " + command.data)
                if(command.eventName == "uncaughtException") do
                    self:kill();
                end
            end
        end
        return true;
    end
    function returnObj:kill(){
        running = false;
        ___NFGS___WRAPPER___TABLE___.kill(self.ptr)
    }
    function returnObj:send(key, data)
        ___NFGS___WRAPPER___TABLE___.send(self.ptr, util.TableToJSON({
            eventName = key,
            data = data
        }))
    end
    return returnObj;
end