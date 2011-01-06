/*--
	Settlement
	Author: Maikel
	
	A nice round to test the settlement objects.
--*/

protected func Initialize()
{
	DoEnvironment();
}

protected func InitializePlayer(int plr)
{ 
	// No FoW needed for now.
	SetFoW(false, plr);
	// Give clonk shovel and a lorry with some equipment.
	var clonk = GetCrew(plr);
	clonk->CreateContents(Shovel);
	var lorry = CreateObject(Lorry, clonk->GetX(), clonk->GetY(), plr);
	lorry->CreateContents(CableReel, 4);
	lorry->CreateContents(Pipe, 2);
	return;
}

private func DoEnvironment()
{
	CreateObject(Environment_Clouds);
	CreateObject(Environment_Celestial);
	CreateObject(Environment_Time);
	Sound("BirdsLoop.ogg",true,100,nil,+1);
}
