---------------------------------------
--
-- Bitfighter geometry library
-- These functions are available for bots, levelgens, and plugins
-- Functions written by Joseph Ivie and Christopher Eykamp
--
---------------------------------------

--[[
@luaclass Geom
@brief    Library of various geometric transforms.
@descr    The %Geom class provides a collection of useful geometric operations.  Since point objects
          are immutable, all the %Geom functions return either a new point, or a new table of new points.  
          All follow the same usage pattern.

In all cases below, \em geom refers to either a single point or a table of points.
--]]

--[[ 
@luafunc point Geom.centroid(geom)
@brief   Find the centroid (center) of `geom`
@param   geom The geometry to find the centroid of
@return  The centroid, the average of all the points in geometry. This point has the property of minimizing the sum square distance to all points in `geom`
 --]]
function Geom.centroid(geom)
    local sum = point.new()
    for _, p in pairs(geom) do
        sum = sum + p
    end
    return sum / #geom
end

-- Helper function
local function flipPoint(p, horizontal)
    if (horizontal) then
        return point.new(-p.x, p.y)
    else
        return point.new(p.x, -p.y)
    end
end

--[[ 
@luafunc geom Geom.flip(geom, horizontal)
@brief   Flip \em geom along the x- or y-axis.
@param   geom - The geometry to modify.  Geom can either be a point or a table of points.
@param   horizontal - Pass true to flip along the x-axis, false to flip along the y-axis.
@return  A geometry of the same type that was passed in.
 --]]
function Geom.flip(geom, horizontal)

    if(type(geom) == 'point') then          -- Single point
        return flipPoint(geom, horizontal)

    else                                    -- Table of points
        local newPoints = {}
        for i = 1, #geom do
            newPoints[i] = flipPoint(geom[i], horizontal)
        end
        return newPoints
    end
end


-- Helper function
local function translatePoint(p, tx, ty)
    return point.new(p.x + tx, p.y + ty)
end

--[[ 
@luafunc Geom.translate(geom, tx, ty)
@brief   Translate (offset) \em geom by \em tx, \em ty.
@param   geom - The geometry to modify.  Geom can either be a point or a table of points.
@param   tx - The amount to add to the x-coord of each point in \em geom.
@param   ty - The amount to add to the y-coord of each point in \em geom.
@return  A geometry of the same type that was passed in.
 --]]
function Geom.translate(geom, tx, ty)
    if(type(geom) == 'point') then          -- Single point
        return translatePoint(geom, tx, ty)
    else                                    -- Table of points
        local newPoints = {}
        for i = 1, #geom do
            newPoints[i] = translatePoint(geom[i], tx, ty)
        end
        return newPoints
    end
end

-- Helper function
local function scalePoint(p, sx, sy, center)
    return point.new((p.x - center.x) * sx, (p.y - center.y) * sy) + center
end

--[[ 
@luafunc Geom.scale(geom, sx, sy)
@brief   Scale \em geom by \em sx, \em sy, in reference to the x- and y-axes.
@param   geom - The geometry to modify.  \em Geom can either be a point or a table of points.
@descr   If \em sy is omitted, \em geom will be scaled evenly horizontally and vertically without distortion.

@param   sx - The amount to scale each point in \em geom horizontally.
@param   sy - (Optional) The amount to scale each point in \em geom vertically.  Defaults to \em sx.
@return  A geometry of the same type that was passed in.
--]]
function Geom.scale(geom, sx, sy)
    sy = sy or sx   -- Passing only one param will scale same amount in x & y directions

    if (type(geom) == 'point') then         -- Single point
        return geom
    else                                    -- Table of points
        local newPoints = {}
        local centroid = Geom.centroid(geom)
        for i = 1, #geom do
            newPoints[i] = scalePoint(geom[i], sx, sy, centroid)
        end
        return newPoints
    end
end


-- Helper function
local function rotatePoint(p, angleRadians, center)
    p = p - center
    local len = point.length(p)
    local pointAngle = math.atan2(p.y, p.x)

    return point.new(len * math.cos(angleRadians + pointAngle), len * math.sin(angleRadians + pointAngle)) + center
end

--[[ 
@luafunc Geom.rotate(geom, angle)
@brief   Rotate \em geom about its centroid.
@param   geom - The geometry to modify.  \em Geom can either be a point or a table of points.
@param   angle - The angle (clockwise, in degrees) to rotate \em geom.
@return  A geometry of the same type that was passed in.
 --]]
function Geom.rotate(geom, angle)
    local angleRadians = angle * math.pi / 180

    if (type(geom) == 'point') then         -- Single point
        return geom
    else                                    -- Table of points
        local centroid = Geom.centroid(geom)
        local newPoints = {}
        for i = 1, #geom do
            newPoints[i] = rotatePoint(geom[i], angleRadians, centroid)
        end
        return newPoints
    end
end


-- Helper function
local function transformPoint(p, tx, ty, sx, sy, angleRadians)
    local len = point.length(p)
    local pointAngle = math.atan2(p.y, p.x)

    return point.new(len * math.cos(angleRadians + pointAngle) * sx + tx, 
                     len * math.sin(angleRadians + pointAngle) * sy + ty )
end

--[[ 
@luafunc Geom.transform(geom, tx, ty, sx, sy, angle)
@brief   Transform \em geom by scaling, rotating, and translating.
@desc    Apply a full transformation to the points in \em geom, doing a combination of the above in a single operation.  
         Scales, rotates, then translates.  Performing these operations together is more effient than applying them
         individually.

Note that \em sy can be omitted for uniform scaling horizontally and vertically.

@param   geom - The geometry to modify.  \em Geom can either be a point or a table of points.
@param   tx - The amount to add to the x-coord of each point in \em geom.
@param   ty - The amount to add to the y-coord of each point in \em geom.
@param   sx - The amount to scale each point in \em geom horizontally.
@param   sy - (Optional) The amount to scale each point in \em geom vertically.  Defaults to \em sx.
@param   angle - The angle (clockwise, in degrees) to rotate \em geom.
@return  A geometry of the same type that was passed in.
 --]]
function Geom.transform(geom, sx, sy, angle, tx, ty)
    -- Check for single scaling factor, and adjust args
    if not ty then
      ty = tx
      tx = sy
      angle = sy  
      sy = sx
    end

    local angleRadians = angle * math.pi / 180

    if (type(geom) == 'point') then         -- Single point
        return transformPoint(geom, tx, ty, sx, sy, angleRadians)
    else                                    -- Table of points
        local newPoints = {}
        for i = 1, #geom do
            newPoints[i] = transformPoint(geom[i], tx, ty, sx, sy, angleRadians)
        end
        return newPoints
    end
end
