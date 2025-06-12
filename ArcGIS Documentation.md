## Route Tool  
**Ballot Collection Routes Notebook**  
**ID:**  
ArcGIS Notebooks

This tool was designed to take in parameters for routing such as open ballot boxes, workers, maximum number of stops, maximum route length and duration, and create optimal routes for ballot collection pickup. The user has the ability to dispatch the created routes to the Snohomish County Ballot Collection Dashboard and batch assign stops to their team in Workforce. The code should be generally user friendly with user inputs clearly labelled and explained before each chunk of code. If at any point, the user decides that the routes created are not useful, assignments can always be created manually in Workforce.

**Things to note:**
- The maximum route duration, length, and maximum number of stops parameters can dramatically impact the routes created and sometimes don’t create optimal routes on the first try. These can always be modified and re-run to create better routes, but be careful not to overuse credits.
- Minor modifications or complete override of routes can happen manually in Workforce at any time. Re-run the cell that removes old routes and stops layers from the map for easier viewing in Workforce and on the Dashboard if the routes are not used.
- The route tool may give Darrington its own route if the parameters aren't resticting enough. This can always be adjusted manually in Workforce.
- The route creation step uses 1 credit for every route created. It’s recommended to save parameters for common scenarios to create good routes on the first run and save credits.
- Adding the workers in by their exact name is very important. Make sure they are already added as Workers in Workforce.
- There is a code chunk at the bottom that deletes old Workforce assignments. Old assignments are hidden from the map but not actually deleted. If things are running slow or issues arise, run this code to clear out old data. This does not need to happen very often.
- ArcGIS Notebooks will allow the kernel to disconnect quickly. To ensure everything runs properly, run all the code quickly and avoid idle time. If it disconnects, you will need to reload and re-run all cells.
- Notebooks does not have an autosave feature, make sure to save your work!

---

## Dashboard  
**Snohomish County Ballot Collection Dashboard**  
**ID:** 0f045666ab1e441aa6aaa0d539b85b29  
ArcGIS Dashboards

A visualization of current routes, collection teams’ progress, and locations. Allows for a simple view of assignment lists and team progress. Once the connection is completed, it also includes a placeholder for smart scale data.

**Things to note:**
- The map visualized and giving data to the lists is the Snohomish County Ballot Collcetion Dispatcher Map (from Workforce). Layers such as the county boundary, ballotboxlocationOD, Stops_Layer, and Routes_Layer are added on.
- Items viewed on the map in Workforce and the Dashboard can be modified or deleted in the Dispatcher Map Map Viewer (you can hide layers or filter for assignment types).
- Dashboard indicators are currently set to filter for TODAY. You can change this if needed.

---

## Workforce  
**Snohomish County Ballot Collection**  
**ID:** 157bf167353e4db3a9e3f739937d0535 (Feature Service)  
ArcGIS Workforce

This tool allows the collection lead to dispatch assignments to collection teams and track their progress. This is not accessible through the folder and must be accessed through Workforce directly. Workers can use the ArcGIS Workforce app on their phones. They will also need to install ArcGIS Survey123 for corresponding surveys at each box.

**Things to note:**
- Workers, assignment types, surveys, and more can be added in the configuration (gear icon) before opening the project. Make sure to update the code if changes are made.
- When a Workforce project is created, it generates three files: a Dispatcher Map, Feature Service, and traditional map. To connect the project to the route tool, use the Feature Service’s ID.
- The survey is currently applied to all assignment types in Workforce.
- Workers need an ArcGIS account to use the app. Only one phone/profile per team is necessary.
- Sometimes the Workforce project needs to be reloaded to reflect updated assignments in the app, this can be done by selecting the three dots before opening the map on the mobile app.
- Items viewed on the map in Workforce and the Dashboard can be modified or deleted in the Dispatcher Map Map Viewer (you can hide layers or filter for assignment types).

---

## Dispatcher Map  
**ArcGIS Web Map**  
**Snohomish County Ballot Collection Dispatcher Map**  
**ID:** 7e59e3112937498aae7024b98cf7013a

This map was created from the Workforce project. Layers for routes, stops, and ballot box locations were added to make them visible in Workforce and on the Dashboard. You can hide or filter layers. These changes are reflected in both Workforce and the Dashboard.

---

## Routes Layer  
**ArcGIS Feature Service**  
**Name:** Routes_Layer  
**ID:** cc05a86529604439b9bf47bf1ad58f69

This is updated by the route tool when new routes are created and is visualized on the Snohomish County Ballot Collection Dispatcher Map.

---

## Stops Layer  
**ArcGIS Feature Service**  
**Name:** Stops_Layer  
**ID:** 1ede5afe0360411a8299a92c869a6040

This is updated by the route tool when new routes are created and is visualized on the Snohomish County Ballot Collection Dispatcher Map.

---

## Survey  
**ArcGIS Survey123**  
**Name:** Ballot Collection Survey FINAL  
**ID:** f72ab5c725964ef09b2ab672d62d5831

The survey is applied to all assignment types in Workforce. The Dashboard also pulls survey data, such as the number of blue ballot tubs collected. Survey data is collected and viewable by those the survey is shared with.

---

## Ballot Box Locations Map  
**ArcGIS Feature Service**  
**Name:** ballotboxlocationOD  
**ID:** 6816bfbff1354d67b0a8f333e1577e5a

This map includes all ballot box locations in Snohomish County. To change or add locations, modify this layer and ensure it’s referenced correctly in the Route Tool using the name as it appears in the file.
