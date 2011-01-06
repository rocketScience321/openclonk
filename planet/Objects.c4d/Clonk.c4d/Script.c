/*
	Clonk
	Author: Randrian

	The protoganist of the game. Witty and nimble if skillfully controled ;-)
*/


// selectable by HUD
#include Library_HUDAdapter
// standard controls
#include Library_ClonkControl
// manager for aiming
#include Library_AimManager

// un-comment them as soon as the new controls work with context menus etc.^
// Context menu
//#include Library_ContextMenu
// Auto production
//#include Library_AutoProduction

// ladder climbing
#include Library_CanClimbLadder

local pInventory;

/* Initialization */

protected func Construction()
{
	_inherited(...);

	SetAction("Walk");
	SetDir(Random(2));
	// Broadcast for rules
	GameCallEx("OnClonkCreation", this);

	AddEffect("IntTurn", this, 1, 1, this);
	AddEffect("IntEyes", this, 1, 35+Random(4), this);
}



/* When adding to the crew of a player */

protected func Recruitment(int iPlr) {
	// Broadcast for crew
	GameCallEx("OnClonkRecruitment", this, iPlr);
	
	return _inherited(iPlr,...);
}

protected func DeRecruitment(int iPlr) {
	// Broadcast for crew
	GameCallEx("OnClonkDeRecruitment", this, iPlr);
	
	return _inherited(iPlr,...);
}


protected func ControlCommand(szCommand, pTarget, iTx, iTy, pTarget2, Data)
{
	// RejectConstruction Callback for building via Drag'n'Drop form a building menu
	// TODO: Check if we still need this with the new system
	if(szCommand == "Construct")
	{
		if(Data->~RejectConstruction(iTx - GetX(), iTy - GetY(), this) )
		{
			return 1;
		}
	}
	// No overloaded command
	return 0;
}


/* Transformation */

public func Redefine(idTo)
{
	// save data of activity
	var phs=GetPhase(),act=GetAction();
	// Transform
	ChangeDef(idTo);
	// restore action
	var chg=SetAction(act);
	if (!chg) SetAction("Walk");
	if (chg) SetPhase(phs);
	// Done
	return 1;
}

/* Events */

protected func CatchBlow()
{
	if (GetAction() == "Dead") return;
	if (!Random(5)) Hurt();
}
	
protected func Hurt()
{
	Sound("Hurt*");
}
	
protected func Grab(object pTarget, bool fGrab)
{
	Sound("Grab");
}

protected func Get()
{
	Sound("Grab");
}

protected func Put()
{
	Sound("Grab");
}

protected func Death(int killed_by)
{
	// this must be done first, before any goals do funny stuff with the clonk
	_inherited(killed_by,...);
	
	// Info-broadcasts for dying clonks.
	GameCallEx("OnClonkDeath", this, killed_by);
	
	// The broadcast could have revived the clonk.
	if (GetAlive())
		return;
	
	Sound("Die");
	DeathAnnounce();
	// If the last crewmember died, do another broadcast.
	if (!GetCrew(GetOwner()))
		GameCallEx("RelaunchPlayer", GetOwner(), killed_by);
	return;
}

protected func Destruction()
{
	_inherited(...);
	// If the clonk wasn't dead yet, he will be now.
	if (GetAlive())
		GameCallEx("OnClonkDeath", this, GetKiller());
	// If this is the last crewmember, do broadcast.
	if (GetCrew(GetOwner()) == this)
	if (GetCrewCount(GetOwner()) == 1)
		// Only if the player is still alive and not yet elimnated.
			if (GetPlayerName(GetOwner()))
				GameCallEx("RelaunchPlayer", GetOwner(), GetKiller());
	return;
}

protected func DeepBreath()
{
	Sound("Breath");
}

protected func CheckStuck()
{
	// Prevents getting stuck on middle vertex
	if(!GetXDir()) if(Abs(GetYDir()) < 5)
		if(GBackSolid(0, 3))
			SetPosition(GetX(), GetY() + 1);
}

/* Status */

// TODO: Make this more sophisticated, readd turn animation and other
// adaptions
public func IsClonk() { return true; }

public func IsJumping(){return GetProcedure() == "FLIGHT";}
public func IsWalking(){return GetProcedure() == "WALK";}

/* Carry items on the clonk */

local iHandMesh;
local fHandAction;
local fBothHanded;

func OnSelectionChanged(int oldslot, int newslot, bool secondaryslot)
{
	AttachHandItem(secondaryslot);
	return _inherited(oldslot, newslot, secondaryslot);
}
func OnSlotEmpty(int slot)
{
	AttachHandItem(slot);
	return _inherited(slot);
}
func OnSlotFull(int slot)
{
	AttachHandItem(slot);
	return _inherited(slot);
}

public func DetachObject(object obj)
{
	if(GetItem(0) == obj)
		DetachHandItem(0);
	if(GetItem(1) == obj)
		DetachHandItem(1);
}

func DetachHandItem(bool secondary)
{
	if(iHandMesh[secondary])
		DetachMesh(iHandMesh[secondary]);
	iHandMesh[secondary] = 0;
}

func AttachHandItem(bool secondary)
{
	if(!iHandMesh) iHandMesh = [0,0];
	DetachHandItem(secondary);
	UpdateAttach();
}

func UpdateAttach()
{
	StopAnimation(GetRootAnimation(6));
	DoUpdateAttach(0);
	DoUpdateAttach(1);
}

func DoUpdateAttach(bool sec)
{
	var obj = GetItem(sec);
	var other_obj = GetItem(!sec);
	if(!obj) return;
	var iAttachMode = obj->~GetCarryMode(this);
	if(iAttachMode == CARRY_None) return;

	if(iHandMesh[sec])
	{
		DetachMesh(iHandMesh[sec]);
		iHandMesh[sec] = 0;
	}

	var bone = "main";
	var bone2;
	if(obj->~GetCarryBone())  bone  = obj->~GetCarryBone(this);
	if(obj->~GetCarryBone2()) bone2 = obj->~GetCarryBone2(this);
	else bone2 = bone;
	var nohand = 0;
	if(!HasHandAction(sec, 1)) nohand = 1;
	var trans = obj->~GetCarryTransform(this, sec, nohand);

	var pos_hand = "pos_hand2";
	if(sec) pos_hand = "pos_hand1";
	var pos_back = "pos_back1";
	if(sec) pos_back = "pos_back2";
	var closehand = "Close2Hand";
	if(sec) closehand = "Close1Hand";

	if(!sec) fBothHanded = 0;

	var special = obj->~GetCarrySpecial(this);
	var special_other;
	if(other_obj) special_other = other_obj->~GetCarrySpecial(this);
	if(special)
	{
		iHandMesh[sec] = AttachMesh(obj, special, bone, trans);
		iAttachMode = 0;
	}

	if(iAttachMode == CARRY_Hand)
	{
		if(HasHandAction(sec, 1))
		{
			iHandMesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
			PlayAnimation(closehand, 6, Anim_Const(GetAnimationLength(closehand)), Anim_Const(1000));
		}
		else
			; // Don't display
	}
	else if(iAttachMode == CARRY_HandBack)
	{
		if(HasHandAction(sec, 1))
		{
			iHandMesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
			PlayAnimation(closehand, 6, Anim_Const(GetAnimationLength(closehand)), Anim_Const(1000));
		}
		else
			iHandMesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
	}
	else if(iAttachMode == CARRY_HandAlways)
	{
		iHandMesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
		PlayAnimation(closehand, 6, Anim_Const(GetAnimationLength(closehand)), Anim_Const(1000));
	}
	else if(iAttachMode == CARRY_Back)
	{
		iHandMesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
	}
	else if(iAttachMode == CARRY_BothHands)
	{
		if(sec) return;
		if(HasHandAction(sec, 1) && !sec && !special_other)
		{
			iHandMesh[sec] = AttachMesh(obj, "pos_tool1", bone, trans);
			PlayAnimation("CarryArms", 6, Anim_Const(obj->~GetCarryPhase(this)), Anim_Const(1000));
			fBothHanded = 1;
		}
		else
			; // Don't display
	}
	else if(iAttachMode == CARRY_Spear)
	{
		if(HasHandAction(sec, 1) && !sec)
		{
			PlayAnimation("CarrySpear", 6, Anim_Const(0), Anim_Const(1000));
		}
		else
			iHandMesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
	}
	else if(iAttachMode == CARRY_Musket)
	{
		if(HasHandAction(sec, 1) && !sec)
		{
			iHandMesh[sec] = AttachMesh(obj, "pos_hand2", bone, trans);
			PlayAnimation("CarryMusket", 6, Anim_Const(0), Anim_Const(1000));
			fBothHanded = 1;
		}
		else
			iHandMesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
	}
	else if(iAttachMode == CARRY_Grappler)
	{
		if(HasHandAction(sec, 1) && !sec)
		{
			iHandMesh[sec] = AttachMesh(obj, "pos_hand2", bone, trans);
			PlayAnimation("CarryCrossbow", 6, Anim_Const(0), Anim_Const(1000));
			fBothHanded = 1;
		}
		else
			iHandMesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
	}
}//AttachMesh(DynamiteBox, "pos_tool1", "main", Trans_Translate(0,0,0));

public func GetHandMesh(object obj)
{
	if(GetItem(0) == obj)
		return iHandMesh[0];
	if(GetItem(1) == obj)
		return iHandMesh[1];
}

static const CARRY_None         = 0;
static const CARRY_Hand         = 1;
static const CARRY_HandBack     = 2;
static const CARRY_HandAlways   = 3;
static const CARRY_Back         = 4;
static const CARRY_BothHands    = 5;
static const CARRY_Spear        = 6;
static const CARRY_Musket       = 7;
static const CARRY_Grappler     = 8;

func HasHandAction(sec, just_wear)
{
	if(sec && fBothHanded)
		return false;
	if(just_wear)
	{
		if( HasActionProcedure() && !fHandAction ) // For wear purpose fHandAction==-1 also blocks
			return true;
	}
	else
	{
		if( HasActionProcedure() && (!fHandAction || fHandAction == -1) )
			return true;
	}
	return false;
}

func HasActionProcedure()
{
	if(GetAction() == "Walk" || GetAction() == "Jump" || GetAction() == "Kneel" || GetAction() == "Ride")
		return true;
	return false;
}

public func ReadyToAction(fNoArmCheck)
{
	if(!fNoArmCheck)
		return HasActionProcedure();
	return HasHandAction(0);
}

public func SetHandAction(bool fNewValue)
{
	if(fNewValue > 0)
		fHandAction = 1; // 1 means can't use items and doesn't draw items in hand
	else if(fNewValue < 0)
		fHandAction = -1; // just don't draw items in hand can still use them
	else
		fHandAction = 0;
	UpdateAttach();
}

public func GetHandAction()
{
	if(fHandAction == 1)
		return true;
	return false;
}

/* Mesh transformations */

local mesh_transformation_list;

func SetMeshTransformation(array transformation, int layer)
{
	if(!mesh_transformation_list) mesh_transformation_list = [];
	if(GetLength(mesh_transformation_list) < layer)
		SetLength(mesh_transformation_list, layer+1);
	mesh_transformation_list[layer] = transformation;
	var all_transformations = 0;
	for(var trans in mesh_transformation_list)
	{
		if(!trans) continue;
		if(all_transformations)
			all_transformations = Trans_Mul(trans, all_transformations);
		else
			all_transformations = trans;
	}
	SetProperty("MeshTransformation", all_transformations);
}

/* Turn */
local iTurnAction;
local iTurnAction2;
local iTurnAction3;

local iTurnKnot1;
local iTurnKnot2;

local iLastTurn;
local iTurnSpecial;

local turn_forced;

static const CLONK_TurnTime = 10;

func SetTurnForced(int dir)
{
	turn_forced = dir+1;
}

func FxIntTurnStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	effect.var0 = GetDirection();
	var iTurnPos = 0;
	if(effect.var0 == COMD_Right) iTurnPos = 1;

	effect.var3 = 24;
//	SetProperty("MeshTransformation", Trans_Rotate(iNumber.var3, 0, 1, 0));
/*
	iTurnAction  = PlayAnimation("TurnRoot120", 1, Anim_Const(iTurnPos*GetAnimationLength("TurnRoot120")), Anim_Const(1000));
	iTurnAction2 = PlayAnimation("TurnRoot180", 1, Anim_Const(iTurnPos*GetAnimationLength("TurnRoot180")), Anim_Const(1000), iTurnAction);
	iTurnKnot1 = iTurnAction2+1;
	iTurnAction3 = PlayAnimation("TurnRoot240", 1, Anim_Const(iTurnPos*GetAnimationLength("TurnRoot240")), Anim_Const(1000), iTurnAction2);
	iTurnKnot2 = iTurnAction3+1;
*/
	effect.var1 = 0;
	effect.var4 = 25;
	effect.var5 = -1;
	SetTurnType(0);
}

func FxIntTurnTimer(pTarget, effect, iTime)
{
	// Check wether the clonk wants to turn (Not when he wants to stop)
	var iRot = effect.var4;
	if(effect.var0 != GetDirection() || effect.var5 != iLastTurn)
	{
		effect.var0 = GetDirection();
		if(effect.var0 == COMD_Right)
		{
			if(iLastTurn == 0)
				iRot = 180-25;
			if(iLastTurn == 1)
				iRot = 180;
		}
		else
		{
			if(iLastTurn == 0)
				iRot = 25;
			if(iLastTurn == 1)
				iRot = 0;
		}
		// Save new ComDir
		effect.var0 = GetDirection();
		effect.var5 = iLastTurn;
		// Notify effects
//		ResetAnimationEffects();
	}
	if(iRot != effect.var3)
	{
		effect.var3 += BoundBy(iRot-effect.var3, -18, 18);
		SetMeshTransformation(Trans_Rotate(effect.var3, 0, 1, 0), 0);
//		SetProperty("MeshTransformation", Trans_Rotate(iNumber.var3, 0, 1, 0));
	}
	effect.var4 = iRot;
	return;
	// Check wether the clonk wants to turn (Not when he wants to stop)
	if(effect.var0 != GetDirection())
	{
		if(effect.var0 == COMD_Right)
		{
			SetAnimationPosition(iTurnAction,  Anim_Linear(GetAnimationLength("TurnRoot120"), GetAnimationLength("TurnRoot120"), 0, CLONK_TurnTime, ANIM_Hold));
			SetAnimationPosition(iTurnAction2, Anim_Linear(GetAnimationLength("TurnRoot180"), GetAnimationLength("TurnRoot180"), 0, CLONK_TurnTime, ANIM_Hold));
			SetAnimationPosition(iTurnAction3, Anim_Linear(GetAnimationLength("TurnRoot240"), GetAnimationLength("TurnRoot240"), 0, CLONK_TurnTime, ANIM_Hold));
		}
		else
		{
			SetAnimationPosition(iTurnAction,  Anim_Linear(0, 0, GetAnimationLength("TurnRoot120"), CLONK_TurnTime, ANIM_Hold));
			SetAnimationPosition(iTurnAction2, Anim_Linear(0, 0, GetAnimationLength("TurnRoot180"), CLONK_TurnTime, ANIM_Hold));
			SetAnimationPosition(iTurnAction3, Anim_Linear(0, 0, GetAnimationLength("TurnRoot240"), CLONK_TurnTime, ANIM_Hold));
		}
		// Save new ComDir
		effect.var0 = GetDirection();
		effect.var1 = CLONK_TurnTime;
		// Notify effects
		ResetAnimationEffects();
	}
	// Turning
	if(effect.var1)
	{
		effect.var1--;
		if(effect.var1 == 0)
		{
			SetAnimationPosition(iTurnAction,  Anim_Const(GetAnimationLength("TurnRoot120")*(GetDirection()==COMD_Right)));
			SetAnimationPosition(iTurnAction2, Anim_Const(GetAnimationLength("TurnRoot180")*(GetDirection()==COMD_Right)));
			SetAnimationPosition(iTurnAction3, Anim_Const(GetAnimationLength("TurnRoot240")*(GetDirection()==COMD_Right)));
		}
	}
}

public func GetTurnPhase()
{
	var iEff = GetEffect("IntTurn", this);
	var iRot = iEff.var3;
	if(iLastTurn == 0)
		return (iRot-25)*100/130;
	if(iLastTurn == 1)
		return iRot*100/180;
	return GetAnimationPosition(iTurnAction)*100/GetAnimationLength("TurnRoot120");
}

local iLastTurn;
local iTurnSpecial;

func SetTurnType(iIndex, iSpecial)
{
	if(iSpecial != nil && iSpecial != 0)
	{
		if(iSpecial == 1) // Start a turn that is forced to the clonk and overwrites the normal action's turntype
			iTurnSpecial = 1;
		if(iSpecial == -1) // Reset special turn (here the iIndex is ignored)
		{
			iTurnSpecial = 0;
			SetTurnType(iLastTurn);
			return;
		}
	}
	else
	{
		// Standart turn? Save and do nothing if we are blocked
		iLastTurn = iIndex;
		if(iTurnSpecial) return;
	}
//	GetEffect("IntTurn", this).var0 = -1;
	return;
	if(iIndex == 0)
	{
		SetAnimationWeight(iTurnKnot1, Anim_Linear(GetAnimationWeight(iTurnKnot1),1000,0,10,ANIM_Hold));
	}
	if(iIndex == 1)
	{
		SetAnimationWeight(iTurnKnot1, Anim_Linear(GetAnimationWeight(iTurnKnot1),0,1000,10,ANIM_Hold));
		SetAnimationWeight(iTurnKnot2, Anim_Linear(GetAnimationWeight(iTurnKnot2),1000,0,10,ANIM_Hold));
	}
	if(iIndex == 2)
	{
		SetAnimationWeight(iTurnKnot2, Anim_Linear(GetAnimationWeight(iTurnKnot2),0,1000,10,ANIM_Hold));
	}
}

// For test purpose
public func TurnFront()
{
	SetAnimationPosition(iTurnAction, Anim_Const(500));
}

func GetDirection()
{
	// Are we forced to a special direction?
	if(turn_forced)
	{
		if(turn_forced == 1) return COMD_Left;
		if(turn_forced == 2) return COMD_Right;
	}
	// Get direction from ComDir
	if(GetAction() != "Scale" && GetAction() != "Jump")
	{
		if(ComDirLike(GetComDir(), COMD_Right)) return COMD_Right;
		else if(ComDirLike(GetComDir(), COMD_Left)) return COMD_Left;
	}
	// if ComDir hasn't a direction, use GetDir
	if(GetDir()==DIR_Right) return COMD_Right;
	else return COMD_Left;
}

/* Animation Manager */

local PropAnimations;

public func ReplaceAction(string action, byaction)
{
	if(PropAnimations == nil) PropAnimations = CreatePropList();
	if(byaction == nil || byaction == 0)
	{
		SetProperty(action, nil, PropAnimations);
		ResetAnimationEffects();
		return true;
	}
/*	if(GetAnimationLength(byaction) == nil)
	{
		Log("ERROR: No animation %s in Definition %s", byaction, GetID()->GetName());
		return false;
	}*/
	if(GetType(byaction) == C4V_Array)
	{
		var old = GetProperty(action, PropAnimations);
		SetProperty(action, byaction, PropAnimations);
		if(GetType(old) == C4V_Array)
		{
			if(ActualReplace == nil) return true;
			if(old[0] == byaction[0] && old[1] == byaction[1])
			{
				var i = 0;
				for (test in ActualReplace)
				{
					if(test && test[0] == action)
						break;
					i++;
				}
				if(i < GetLength(ActualReplace))
					SetAnimationWeight(ActualReplace[i][1], Anim_Const(byaction[2]));
				return true;
			}
		}
	}
	SetProperty(action, byaction, PropAnimations);
//	if(ActualReplace != nil)
//		SetAnimationWeight(ActualReplace, Anim_Const(byaction[2]));
	ResetAnimationEffects();
	return true;
}

public func ResetAnimationEffects()
{
	if(GetEffect("IntWalk", this))
		EffectCall(this, GetEffect("IntWalk", this), "Reset");
	if(GetAction() == "Jump")
		StartJump();
}

local ActualReplace;

public func PlayAnimation(string animation, int index, array position, array weight, int sibling)
{
	if(!ActualReplace) ActualReplace = [];
	ActualReplace[index] = nil;
	if(PropAnimations != nil)
		if(GetProperty(animation, PropAnimations) != nil)
		{
			var replacement = GetProperty(animation, PropAnimations);
			if(GetType(replacement) == C4V_Array)
			{
				var animation1 = inherited(replacement[0], index, position, weight);
				var animation2 = inherited(replacement[1], index, position, Anim_Const(500), animation1);
				var animationKnot = animation2 + 1;
				ActualReplace[index] = [animation, animationKnot];
				SetAnimationWeight(animationKnot, Anim_Const(replacement[2]));
				return animation1;
			}
			else
				animation = GetProperty(animation, PropAnimations);
		}
	return inherited(animation, index, position, weight, sibling, ...);
}

public func GetAnimationLength(string animation)
{
	var replacement;
	if(PropAnimations != nil)
		if(replacement = GetProperty(animation, PropAnimations))
		{
			if(GetType(replacement) == C4V_Array)
				animation = replacement[0];
			else
				animation = replacement;
		}
	return inherited(animation, ...);
}

/* Eyes */
func FxIntEyesTimer(target, effect, time)
{
	if(!Random(4))
		AddEffect("IntEyesClosed", this, 10, 6, this);
}

func FxIntEyesClosedStart(target, effect, tmp)
{
	CloseEyes(1);
}

func FxIntEyesClosedStop(target, effect, reason, tmp)
{
	CloseEyes(-1);
}

local closed_eyes;
func CloseEyes(iCounter)
{
	StopAnimation(GetRootAnimation(3));
//	PlayAnimation("CloseEyes", 6, Anim_Linear(0, 0, GetAnimationLength("CloseEyes")/2, 3, ANIM_Hold), Anim_Const(1000));
	closed_eyes += iCounter;
	if(closed_eyes >= 1)
		PlayAnimation("CloseEyes" , 3, Anim_Linear(0, 0, GetAnimationLength("CloseEyes")/2, 3, ANIM_Hold), Anim_Const(1000));
//		SetMeshMaterial("Clonk_Body_EyesClosed");
	else
		PlayAnimation("CloseEyes" , 3, Anim_Linear(GetAnimationLength("CloseEyes")/2, GetAnimationLength("CloseEyes")/2, GetAnimationLength("CloseEyes"), 3, ANIM_Remove), Anim_Const(1000));
//		SetMeshMaterial("Clonk_Body");
}

/* Walking backwards */
func SetBackwardsSpeed(int value)
{
	BackwardsSpeed = value;
	UpdateBackwardsSpeed();
}

local BackwardsSpeed;
local Backwards;

func UpdateBackwardsSpeed()
{
	if(GetComDir() != GetDirection() && Backwards != 1 && BackwardsSpeed != nil)
	{
		AddEffect("IntWalkBack", this, 1, 0, this, 0, BackwardsSpeed);
		Backwards = 1;
	}
	if( (GetComDir() == GetDirection() && Backwards == 1) || BackwardsSpeed == nil)
	{
		RemoveEffect("IntWalkBack", this);
		Backwards = nil;
	}
}

func FxIntWalkBackStart(pTarget, effect, fTmp, iValue)
{
	if(iValue == nil) iValue = 84;
	pTarget->PushActionSpeed("Walk", iValue);
}

func FxIntWalkBackStop(pTarget, effect)
{
	pTarget->PopActionSpeed("Walk");
}

/* Walk */

static const Clonk_WalkInside = "Inside";
static const Clonk_WalkStand = "Stand";
static const Clonk_WalkWalk  = "Walk";
static const Clonk_WalkRun   = "Run";
static Clonk_IdleActions;

func StartWalk()
{
	if(Clonk_IdleActions == nil)
		Clonk_IdleActions = [["IdleLookAround", 60], ["IdleHandwatch", 100], ["IdleScratch", 70], ["IdleStrech", 100], ["IdleShoe", 120], ["IdleShoeSole", 200], ["IdleHandstrech", 100]];
	if(!GetEffect("IntWalk", this))
		AddEffect("IntWalk", this, 1, 1, this);
}

func StopWalk()
{
	if(GetAction() != "Walk") RemoveEffect("IntWalk", this);
}

func GetCurrentWalkAnimation()
{
	if(Contained())
		return Clonk_WalkInside;
	else SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,1000,5000), Trans_Rotate(70,0,1,0)), this);
	var velocity = Distance(0,0,GetXDir(),GetYDir());
	if(velocity < 1) return Clonk_WalkStand;
	if(velocity < 10) return Clonk_WalkWalk;
	return Clonk_WalkRun;
}

func GetWalkAnimationPosition(string anim, int pos)
{
	var dir = -1;
	if(GetDirection() == COMD_Right) dir = +1;
	if(PropAnimations != nil)
		if(GetProperty(Format("%s_Position", anim), PropAnimations))
		{
			var length = GetAnimationLength(anim);
			if(GetProperty(anim, PropAnimations)) length = GetAnimationLength(GetProperty(anim, PropAnimations));
			return Anim_X(pos, 0, length, GetProperty(Format("%s_Position", anim), PropAnimations)*dir);
		}
	// TODO: Choose proper starting positions, depending on the current
	// animation and its position: For Stand->Walk or Stand->Run, start
	// with a frame where one of the clonk's feets is on the ground and
	// the other one is in the air. For Walk->Run and Run->Walk, fade to
	// a state where its feets are at a similar position (just taking
	// over previous animation's position might work, using
	// GetAnimationPosition()). Walk->Stand is arbitrary I guess.
	// First parameter of Anim_Linear/Anim_AbsX is initial position.
	// Movement synchronization might also be tweaked somewhat as well.
	if(anim == Clonk_WalkInside)
		return Anim_Const(0);
	if(anim == Clonk_WalkStand)
		return Anim_Linear(pos, 0, GetAnimationLength(anim), 35, ANIM_Loop);
	else if(anim == Clonk_WalkWalk)
		return Anim_X(pos, 0, GetAnimationLength(anim), 20*dir);
	else if(anim == Clonk_WalkRun)
		return Anim_X(pos, 0, GetAnimationLength(anim), 50*dir);
}

func FxIntWalkStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	// Always start in Stand for now... should maybe fade properly from previous animation instead
	var anim = "Stand";  //GetCurrentWalkAnimation();
	effect.var0 = anim;
	effect.var1 = PlayAnimation(anim, 5, GetWalkAnimationPosition(anim), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	effect.var2 = 0;

	effect.var3 = 0; // Idle counter
	effect.var5 = Random(300); // Random offset for idle time
	// Update carried items
	UpdateAttach();
	// Set proper turn
	SetTurnType(0);
}

func FxIntWalkTimer(pTarget, effect)
{
/*	if(iNumber.var4)
	{
		iNumber.var4--;
		if(iNumber.var4 == 0)
			SetAnimationPosition(iTurnAction, Anim_Const(1200*(GetDirection()==COMD_Right)));
	}*/
	if(BackwardsSpeed != nil)
		UpdateBackwardsSpeed();
	if(effect.var2)
	{
		effect.var2--;
		if(effect.var2 == 0)
			effect.var0 = 0;
	}
	var anim = GetCurrentWalkAnimation();
	if(anim != effect.var0 && !effect.var4)
	{
		effect.var0 = anim;
		effect.var3 = 0;
		effect.var1 = PlayAnimation(anim, 5, GetWalkAnimationPosition(anim, 0), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	}
	// The clonk has to stand, not making a pause animation yet and not doing other actions with the hands (e.g. loading the bow)
	else if(anim == Clonk_WalkStand && !GetHandAction())
	{
		if(!effect.var2)
		{
			effect.var3++;
			if(effect.var3 > 300+effect.var5)
			{
				effect.var3 = 0;
				effect.var5 = Random(300);
				var rand = Random(GetLength(Clonk_IdleActions));
				PlayAnimation(Clonk_IdleActions[rand][0], 5, Anim_Linear(0, 0, GetAnimationLength(Clonk_IdleActions[rand][0]), Clonk_IdleActions[rand][1], ANIM_Remove), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
				effect.var2 = Clonk_IdleActions[rand][1]-5;
			}
		}
	}
	else
	{
		effect.var3 = 0;
		if(effect.var2)
		{
			effect.var0 = 0;
			effect.var2 = 0;
		}
	}
/*	// Check wether the clonk wants to turn (Not when he wants to stop)
	if(iNumber.var17 != GetDirection())
	{
		var iTurnTime = 10;
//		if(Distance(0,0,GetXDir(),GetYDir()) < 10) //TODO Run turn animation
//		{
		if(iNumber.var17 == COMD_Right)
		{
			iNumber.var0 = PlayAnimation("StandTurn", 5, Anim_Linear(0, 0, 2000, iTurnTime, ANIM_Hold), Anim_Linear(0, 0, 1000, 2, ANIM_Remove));
			SetAnimationPosition(iTurnAction, Anim_Linear(1200, 1200, 0, iTurnTime, ANIM_Hold));
		}
		else
		{
			iNumber.var0 = PlayAnimation("StandTurn", 5, Anim_Linear(3000, 3000, 5000, iTurnTime, ANIM_Hold), Anim_Linear(0, 0, 1000, 2, ANIM_Remove));
			SetAnimationPosition(iTurnAction, Anim_Linear(0, 0, 1200, iTurnTime, ANIM_Hold));
		}
//		}
		//else
		//	iNumber.var0 = PlayAnimation("RunTurn", 5, Anim_Linear(0, 0, 2400, iTurnTime, ANIM_Hold), Anim_Linear(0, 0, 1000, 2, ANIM_Remove));
		// Save new ComDir
		iNumber.var17 = GetDirection();
		iNumber.var4 = iTurnTime;
	}*/
}

func FxIntWalkReset(pTarget, effect)
{
	effect.var0 = 0;
}

func StartStand()
{
	PlayAnimation(Clonk_WalkStand, 5, GetWalkAnimationPosition(Clonk_WalkStand), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(0);
}

/*
func FxIntWalkStart(pTarget, iNumber, fTmp)
{
	if(fTmp) return;
	for(var i = 0; i < GetLength(Clonk_WalkStates); i++)
		AnimationPlay(Clonk_WalkStates[i], 0);
	iNumber.var0 = 0; // Phase
	iNumber.var1 = 1000; // Stand weight
	iNumber.var2 = 0; // Walk weight
	iNumber.var3 = 0; // Run weight
	iNumber.var4 = 0;
	iNumber.var5 = 0;
	iNumber.var14 = 0; // Oldstate
	iNumber.var15 = 0; // Save wether the last frame was COMD_Stop

	iNumber.var17 = GetComDir(); // OldDir
	if(GetComDir() == COMD_Stop)
	{
		if(GetDir()) iNumber.var17 = COMD_Right;
		else iNumber.var17 = COMD_Left;
	}
	iNumber.var18 = 0; // Turn Phase
	iNumber.var19 = 0; // Wether to use Run turn or not
}

func FxIntWalkStop(pTarget, iNumber, iReason, fTmp)
{
	if(fTmp) return;
	for(var i = 0; i < GetLength(Clonk_WalkStates); i++)
		AnimationStop(Clonk_WalkStates[i]);
}

func FxIntWalkTimer(pTarget, iNumber, iTime)
{
	var iSpeed = Distance(0,0,GetXDir(),GetYDir());
	var iState = 0;

	// Check wether the clonk wants to turn
	if(iNumber.var17 != GetComDir())
	{
		// Not when he wants to stop
		if(GetComDir()!= COMD_Stop)
		{
			// Save new ComDir and start turn
			iNumber.var17 = GetComDir();
			iNumber.var18 = 1;
			// The weight of run and stand goes to their turning actions
			iNumber.var5 = iNumber.var3;
			iNumber.var3 = 0;
			iNumber.var4 = iNumber.var1;
			iNumber.var1 = 0;
			// Decide wether to use StandTurn or RunTurn
			if(iSpeed < 10)
				iNumber.var19 = 0;
			else
				iNumber.var19 = 1;
		}
	}
	// Turning
	if(iNumber.var18)
	{
		// Play animations
		AnimationSetState("StandTurn", iNumber.var18*100, nil);
		AnimationSetState("RunTurn", iNumber.var18*100, nil);
		//
		if( ( iNumber.var17 == COMD_Left && GetDir() )
			|| ( iNumber.var17 == COMD_Right && !GetDir() ) )
			{
				SetObjDrawTransform(-1000, 0, 0, 0, 1000);
				//AnimationSetState("RunTurn", iNumber.var18*100+2400, nil);
			}
			else SetObjDrawTransform(1000, 0, 0, 0, 1000);
		iNumber.var18 += 2;
		if(iNumber.var18 >= 24)
			iNumber.var18 = 0;
		iState = 4 + iNumber.var19;
	}
	// Play stand animation when not moving
	else if(iSpeed < 1 && iNumber.var15)
	{
		AnimationSetState("Stand", ((iTime/5)%11)*100, nil);
		iState = 1;
	}
	// When moving slowly play synchronized with movement walk
	else if(iSpeed < 10)
	{
		iNumber.var0 +=  iSpeed*25/(16*1);
		if(iNumber.var0 > 250) iNumber.var0 -= 250;

		AnimationSetState("Walk", iNumber.var0*10, nil);
		iState = 2;
	}
	// When moving fast play run
	else
	{
		if(iNumber.var14 != 3)
		{
			if(iNumber.var14 == 5)
				iNumber.var0 = 60; // start with frame 190 (feet on the floor)
			else
				iNumber.var0 = 190; // start with frame 190 (feet on the floor)
		}
		else
			iNumber.var0 += iSpeed*25/(16*3);
		if(iNumber.var0 > 250) iNumber.var0 -= 250;

		AnimationSetState("Run", iNumber.var0*10, nil);
		iState = 3;
	}

	// Save wether he have COMD_Stop or not. So a single frame with COMD_Stop keeps the movement
	if(GetComDir() == COMD_Stop) iNumber.var15 = 1;
	else iNumber.var15 = 0;
	
	// Blend between the animations: The actuall animations gains weight till it reaches 1000
	// the other animations lose weight until they are at 0
	for(var i = 1; i <= 5; i++)
	{
		if(i == iState)
		{
			if(EffectVar(i, pTarget, iNumber) < 1000)
				EffectVar(i, pTarget, iNumber) += 200;
		}
		else
		{
			if(EffectVar(i, pTarget, iNumber) > 0)
				EffectVar(i, pTarget, iNumber) -= 200;
		}
		AnimationSetState(Clonk_WalkStates[i-1], nil, EffectVar(i, pTarget, iNumber));
	}
	iNumber.var14 = iState;
}
*/


/* Scale */

func StartScale()
{
	if(!GetEffect("IntScale", this))
		AddEffect("IntScale", this, 1, 1, this);
	// Set proper turn type
	SetTurnType(1);
	// Update carried items
	UpdateAttach();
}

func StopScale()
{
	if(GetAction() != "Scale") RemoveEffect("IntScale", this);
}

func FxIntScaleStart(target, effect, tmp)
{
	if(tmp) return;
	PlayAnimation("Scale", 5, Anim_Y(0, GetAnimationLength("Scale"), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	effect.var0 = -1;
	effect.var3 = 0;
//	FxIntScaleTimer(target, number, 0);
}

func CheckPosition(int off_x, int off_y)
{
	var free = 1;
	SetPosition(GetX()+off_x, GetY()+off_y);
	if(Stuck()) free = 0;
	SetPosition(GetX()-off_x, GetY()-off_y);
	return free;
}

func CheckScaleTop()
{
	// Test whether the clonk has reached a top corner
	if(GBackSolid(-8+16*GetDir(),-8)) return false;
	if(!CheckPosition(-7*(-1+2*GetDir()),-17)) return false;
	return true;
}

func FxIntScaleStart(target, effect, tmp)
{
	if(tmp) return;
	effect.var1 = PlayAnimation("Scale", 5, Anim_Y(0, GetAnimationLength("Scale"), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	effect.var0 = 0;
}

func FxIntScaleTimer(target, effect, time)
{
	if(GetAction() != "Scale") return;
	// When the clonk reaches the top play an extra animation
/*	if(CheckScaleTop())
	{
		// If the animation is not already set
		if(number.var0 != 1)
		{
			var dist = 0;
			while(!GBackSolid(-8+16*GetDir(),dist-8) && dist < 7) dist++;
			number.var1 = PlayAnimation("ScaleTop", 5, Anim_Linear(GetAnimationLength("ScaleTop")*dist/10,0, GetAnimationLength("ScaleTop"), 20, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			number.var0 = 1;
			number.var2 = COMD_Up;
		}
		// The animation's graphics has to be shifet a bit to adjust to the clonk movement
		var pos = GetAnimationPosition(number.var1);
		var percent = pos*1000/GetAnimationLength("ScaleTop");
		SetObjDrawTransform(1000, 0, 3*(-1+2*GetDir())*percent, 0, 1000, 3*percent);
		// If the Comdir has changed...
		if(number.var2 != GetComDir())
		{
			// Go on if the user has stopped. Stopping here doesn't look good
			if(GetComDir() == COMD_Stop || GetComDir() == -1)
			{
				SetComDir(number.var2);
				number.var3 = 1;
			}
			// Or adjust the animation to the turn of direction
			else
			{
				number.var3 = 0;
				var anim = Anim_Linear(pos,0, GetAnimationLength("ScaleTop"), 20, ANIM_Hold);
				if(ComDirLike(GetComDir(), COMD_Down))
					anim = Anim_Linear(pos,0, GetAnimationLength("ScaleTop"),-20, ANIM_Hold);
				SetAnimationPosition(number.var1, anim);
				number.var2 = GetComDir();
			}
		}
	}
	else if(!GBackSolid(-6+14*GetDir(), 6))
	{
		if(number.var0 != 2)
		{
			var pos = GetAnimationPosition(number.var1);
			number.var1 = PlayAnimation("ScaleHands" , 5, Anim_Y(pos, GetAnimationLength("ScaleHands"), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			number.var4 = PlayAnimation("ScaleHands2", 5, Anim_Y(pos, GetAnimationLength("ScaleHands2"), 0, 15), Anim_Const(1000), number.var1);
			number.var4++;
//			SetAnimationWeight(number.var4, Anim_Const(Cos(time, 1000)));
			number.var0 = 2;
		}
		SetAnimationWeight(number.var4, Anim_Const(Cos(time*2, 500)+500));
	}
	// If not play the normal scale animation
	else if(number.var0 != 0)
	{
		SetObjDrawTransform(1000, 0, 0, 0, 1000, 0);
		if(number.var3)
		{
			SetComDir(COMD_Stop);
			number.var3 = 0;
		}
		var pos = 0;
		if(number.var0 == 2) pos = GetAnimationPosition(number.var1);
		number.var1 = PlayAnimation("Scale", 5, Anim_Y(0, GetAnimationLength("Scale"), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		number.var0 = 0;
	}
	if(number.var0 == 0)
	{
		var x, x2;
		var y = -7, y2 = 8;
		for(x = 0; x < 10; x++)
			if(GBackSolid(x*(-1+2*GetDir()), y)) break;
		for(x2 = 0; x2 < 10; x2++)
			if(GBackSolid(x2*(-1+2*GetDir()), y2)) break;
		var angle = Angle(x2, y2, x, y)*(+1-2*GetDir());
		var mid = (x+x2)*1000/2 - 5000;
		SetScaleRotation(angle, mid*(-1+2*GetDir()));
	}*/
}
/*
func SetScaleRotation (int r, int xoff, int yoff) {
	var fsin=Sin(r, 1000), fcos=Cos(r, 1000);
	// set matrix values
	SetObjDrawTransform (
		+fcos, +fsin, xoff, //(1000-fcos)*xoff - fsin*yoff,
		-fsin, +fcos, yoff, //(1000-fcos)*yoff + fsin*xoff,
	);
}*/

func FxIntScaleStop(target, effect, reason, tmp)
{
	if(tmp) return;
/*	// Set the animation to stand without blending! That's cause the animation of Scale moves the clonkmesh wich would result in a stange blend moving the clonk around while blending
	if(number.var0 == 1) PlayAnimation(Clonk_WalkStand, 5, GetWalkAnimationPosition(Clonk_WalkStand), Anim_Const(1000));
	// Finally stop if the user has scheduled a stop
	if(number.var3) SetComDir(COMD_Stop);
	// and reset the transform
	SetObjDrawTransform(1000, 0, 0, 0, 1000, 0);*/
}

// This is just for test... TODO RemoveMe
func NameComDir(comdir)
{
	if(comdir == COMD_Stop) return "COMD_Stop";
	if(comdir == COMD_Up) return "COMD_Up";
	if(comdir == COMD_UpRight) return "COMD_UpRight";
	if(comdir == COMD_UpLeft) return "COMD_UpLeft";
	if(comdir == COMD_Right) return "COMD_Right";
	if(comdir == COMD_Left) return "COMD_Left";
	if(comdir == COMD_Down) return "COMD_Down";
	if(comdir == COMD_DownRight) return "COMD_DownRight";
	if(comdir == COMD_DownLeft) return "COMD_DownLeft";
	if(comdir == COMD_None) return "COMD_None";
}

/* Jump */

func StartJump()
{
	// TODO: Tweak animation speed
	PlayAnimation("Jump", 5, Anim_Linear(0, 0, GetAnimationLength("Jump"), 8*3, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(0);
	// TODO: make dive animation and uncomment here
//	var iX=GetX(),iY=GetY(),iXDir=GetXDir(),iYDir=GetYDir();
//		if (SimFlight(iX,iY,iXDir,iYDir,25)) // SimFlight behavior changed. (10/1/10)
//			if (GBackLiquid(iX-GetX(),iY-GetY()) && GBackLiquid(iX-GetX(),iY+GetDefHeight()/2-GetY()))
//				PlayAnimation("Dive", 5, Anim_Linear(0, 0, GetAnimationLength("Dive"), 8*3, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));;
	if(!GetEffect("Fall", this))
		AddEffect("Fall",this,1,1,this);
}

func FxFallEffect(string new_name, object target)
{
	// reject more than one fall effects.
	if(new_name == "Fall") return -1;
}

func FxFallTimer(object target, effect, int timer)
{
	if(GetYDir() > 55 && GetAction() == "Jump")
	{
		PlayAnimation("Fall", 5, Anim_Linear(0, 0, GetAnimationLength("Fall"), 8*3, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		return -1;
	}
	if(GetAction() != "Jump")
		return -1;
}

/* Hangle */

/* Replaces the named action by an instance with a different speed */
func PushActionSpeed(string action, int n)
{
	if (ActMap == this.Prototype.ActMap)
		ActMap = { Prototype = this.Prototype.ActMap };
	ActMap[action] = { Prototype = ActMap[action], Speed = n };
	if (this.Action == ActMap[action].Prototype)
		this.Action = ActMap[action];
}

/* Resets the named action to the previous one */
func PopActionSpeed(string action, int n) {
	// FIXME: This only works if PushActionSpeed and PopActionSpeed are the only functions manipulating the ActMap
	if (this.Action == ActMap[action])
		this.Action = ActMap[action].Prototype;
	ActMap[action] = ActMap[action].Prototype;
}

func StartHangle()
{
/*	if(Clonk_HangleStates == nil)
		Clonk_HangleStates = ["HangleStand", "Hangle"];*/
	if(!GetEffect("IntHangle", this))
		AddEffect("IntHangle", this, 1, 1, this);
	// Set proper turn type
	SetTurnType(1);
	// Update carried items
	UpdateAttach();
}

func StopHangle()
{
	if(GetAction() != "Hangle") RemoveEffect("IntHangle", this);
}

func FxIntHangleStart(pTarget, effect, fTmp)
{
	effect.var10 = ActMap.Hangle.Speed;
	PushActionSpeed("Hangle", effect.var10);
	if(fTmp) return;

	// EffectVars:
	// 0: whether the clonk is currently moving or not (<=> current animation is Hangle or HangleStand)
	// 1: Current animation number
	// 6: Player requested the clonk to stop
	// 7: Whether the HangleStand animation is shown front-facing or back-facing
	// 10: Previous Hangle physical

	effect.var1 = PlayAnimation("HangleStand", 5, Anim_Linear(0, 0, 2000, 100, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

}

func FxIntHangleStop(pTarget, effect, iReasonm, fTmp)
{
	PopActionSpeed("Hangle");
	if(fTmp) return;
}

func FxIntHangleTimer(pTarget, effect, iTime)
{
	// (TODO: Instead of iNumber.var0 we should be able
	// to query the current animation... maybe via a to-be-implemented
	// GetAnimationName() engine function.

	// If we are currently moving
	if(effect.var0)
	{
		// Use a cosine-shaped movement speed (the clonk only moves when he makes a "stroke")
		var iSpeed = 50-Cos(GetAnimationPosition(effect.var1)/10*360*2/1000, 50);
		ActMap.Hangle.Speed = effect.var10*iSpeed/50;

		// Exec movement animation (TODO: Use Anim_Linear?)
		var position = GetAnimationPosition(effect.var1);
		position += (effect.var10*5/48*1000/(14*2));

		SetAnimationPosition(effect.var1, Anim_Const(position % GetAnimationLength("Hangle")));

		// Continue movement, if the clonk still has momentum
		if(GetComDir() == COMD_Stop && iSpeed>10)
		{
			// Make it stop after the current movement
			effect.var6 = 1;

			if(GetDir())
				SetComDir(COMD_Right);
			else
				SetComDir(COMD_Left);
		}
		// Stop movement if the clonk has lost his momentum
		else if(iSpeed <= 10 && (GetComDir() == COMD_Stop || effect.var6))
		{
			effect.var6 = 0;
			SetComDir(COMD_Stop);

			// and remeber the pose (front or back)
			if(GetAnimationPosition(effect.var1) > 2500 && GetAnimationPosition(effect.var1) < 7500)
				effect.var7 = 1;
			else
				effect.var7 = 0;

			// Change to HangleStand animation
			var begin = 4000*effect.var7;
			var end = 2000+begin;
			effect.var1 = PlayAnimation("HangleStand", 5, Anim_Linear(begin, begin, end, 100, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			effect.var0 = 0;
		}
	}
	else
	{
		// We are currently not moving
		if(GetComDir() != COMD_Stop)
		{
			// Switch to move
			effect.var0 = 1;
			// start with frame 100 or from the back hanging pose frame 600
			var begin = 10*(100 + 500*effect.var7);
			effect.var1 = PlayAnimation("Hangle", 5, Anim_Const(begin), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		}
	}
}

/* Swim */

func StartSwim()
{
/*	if(Clonk_SwimStates == nil)
		Clonk_SwimStates = ["SwimStand", "Swim", "SwimDive", "SwimTurn", "SwimDiveTurn", "SwimDiveUp", "SwimDiveDown"];*/
	if(!GetEffect("IntSwim", this))
		AddEffect("IntSwim", this, 1, 1, this);
	SetVertex(1,VTX_Y,-4,2);
}

func StopSwim()
{
	if(GetAction() != "Swim") RemoveEffect("IntSwim", this);
	SetVertex(1,VTX_Y,-7,2);
}

func FxIntSwimStart(pTarget, effect, fTmp)
{
	if(fTmp) return;

	effect.var0 = "SwimStand";
	effect.var1 = PlayAnimation("SwimStand", 5, Anim_Linear(0, 0, GetAnimationLength("SwimStand"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
/*
	for(var i = 0; i < GetLength(Clonk_SwimStates); i++)
	AnimationPlay(Clonk_SwimStates[i], 0);
	iNumber.var0 = 0; // Phase
	iNumber.var1 = 1000; // Stand weight
	iNumber.var2 = 0; // Walk weight
	iNumber.var3 = 0; // Run weight
	iNumber.var4 = 0; // Oldstate
	iNumber.var5 = 0; // Save wether the last frame was COMD_Stop
	iNumber.var6 = 0; // OldRot

	iNumber.var7 = GetDir(); // OldDir
	iNumber.var8 = 0; // Turn Phase
	AnimationSetState("SwimStand", 0, 1000);*/

	// Set proper turn type
	SetTurnType(0);
	// Update carried items
	UpdateAttach();
	SetAnimationWeight(iTurnKnot2, Anim_Const(1000));
}

func FxIntSwimTimer(pTarget, effect, iTime)
{
//	DoEnergy(1); //TODO Remove this! Endless Energy while diving is only for the testers

	var iSpeed = Distance(0,0,GetXDir(),GetYDir());

	// TODO: Smaller transition time between dive<->swim, keep 15 for swimstand<->swim/swimstand<->dive

	// Play stand animation when not moving
	if(Abs(GetXDir()) < 1 && !GBackSemiSolid(0, -5))
	{
		if (GetContact(-1) & CNAT_Bottom)
		{
			SetAction("Walk");
			return -1;
		}
		if(effect.var0 != "SwimStand")
		{
			effect.var0 = "SwimStand";
			effect.var1 = PlayAnimation("SwimStand", 5, Anim_Linear(0, 0, GetAnimationLength("SwimStand"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
		}
		SetAnimationWeight(iTurnKnot1, Anim_Const(0));
	}
	// Swimming
	else if(!GBackSemiSolid(0, -5))
	{
		// Animation speed by X
		if(effect.var0 != "Swim")
		{
			effect.var0 = "Swim";
			// TODO: Determine starting position from previous animation
			PlayAnimation("Swim", 5, Anim_AbsX(0, 0, GetAnimationLength("Swim"), 25), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
		}
		SetAnimationWeight(iTurnKnot1, Anim_Const(0));
	}
	// Diving
	else
	{
		if(effect.var0 != "SwimDive")
		{
			effect.var0 = "SwimDive";
			// TODO: Determine starting position from previous animation
			effect.var2 = PlayAnimation("SwimDiveUp", 5, Anim_Linear(0, 0, GetAnimationLength("SwimDiveUp"), 40, ANIM_Loop), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
			effect.var3 = PlayAnimation("SwimDiveDown", 5, Anim_Linear(0, 0, GetAnimationLength("SwimDiveDown"), 40, ANIM_Loop), Anim_Const(500), effect.var2);
			effect.var1 = effect.var3 + 1;

			// TODO: This should depend on which animation we come from
			// Guess for SwimStand we should fade from 0, otherwise from 90.
			effect.var4 = 90;
		}

		if(iSpeed)
		{
			var iRot = Angle(-Abs(GetXDir()), GetYDir());
			effect.var4 += BoundBy(iRot - effect.var4, -4, 4);
		}

		// TODO: Shouldn't weight go by sin^2 or cos^2 instead of linear in angle?
		var weight = 1000*effect.var4/180;
		SetAnimationWeight(effect.var1, Anim_Const(1000 - weight));
		SetAnimationWeight(iTurnKnot1, Anim_Const(1000 - weight));
	}
}

/*
func FxIntScaleStart(pTarget, iNumber, fTmp)
{
	if(fTmp) return;
	AnimationPlay("Scale", 1000);
	iNumber.var0 = 0; // Phase
	iNumber.var1 = 1000; // Stand weight
	iNumber.var2 = 0; // Walk weight
	iNumber.var3 = 0; // Run weight
	iNumber.var4 = 0; // Oldstate
	iNumber.var5 = 0; // Save wether the last frame was COMD_Stop
}

func FxIntScaleStop(pTarget, iNumber, iReason, fTmp)
{
	if(fTmp) return;
	AnimationStop("Scale");
}

func FxIntScaleTimer(pTarget, iNumber, iTime)
{
//	if(GetAction() != "Walk") return -1;
	var iSpeed = -GetYDir();
	var iState = 0;

	// Play stand animation when not moving
	if(iSpeed < 1 && iNumber.var5)
	{
//		AnimationSetState("Stand", ((iTime/5)%11)*100, nil);
		iState = 2;
	}
	// When moving slowly play synchronized with movement walk
	else
	{
		iNumber.var0 +=  iSpeed*20/(16*1);
		if(iNumber.var0 < 0) iNumber.var0 += 200;
		if(iNumber.var0 > 200) iNumber.var0 -= 200;

		AnimationSetState("Scale", iNumber.var0*10, nil);
		iState = 2;
	}

	// Save wether he have COMD_Stop or not. So a single frame with COMD_Stop keeps the movement
	if(GetComDir() == COMD_Stop) iNumber.var5 = 1;
	else iNumber.var5 = 0;

	// Blend between the animations: The actuall animations gains weight till it reaches 1000
	// the other animations lose weight until they are at 0
	for(var i = 1; i <= 1; i++)
	{
		if(i == iState)
		{
			if(EffectVar(i, pTarget, iNumber) < 1000)
				EffectVar(i, pTarget, iNumber) += 200;
		}
		else
		{
			if(EffectVar(i, pTarget, iNumber) > 0)
				EffectVar(i, pTarget, iNumber) -= 200;
		}
//		AnimationSetState(Clonk_WalkStates[i-1], nil, EffectVar(i, pTarget, iNumber));
	}
	iNumber.var4 = iState;
}*/

func Hit(int iXSpeed, int iYSpeed)
{
	if(iYSpeed < 450) return;
	if(GetAction() != "Walk") return;
	var iKneelDownSpeed = 18;
	SetXDir(0);
	SetAction("Kneel");
	PlayAnimation("KneelDown", 5, Anim_Linear(0, 0, GetAnimationLength("KneelDown"), iKneelDownSpeed, ANIM_Remove), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	ScheduleCall(this, "EndKneel", iKneelDownSpeed, 1);
}

func EndKneel()
{
	SetAction("Walk");
}

func StartDigging()
{
	if(!GetEffect("IntDig", this))
		AddEffect("IntDig", this, 1, 1, this);
}

func StopDigging()
{
	if(GetAction() != "Dig") RemoveEffect("IntDig", this);
}

func FxIntDigStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	effect.var1 = PlayAnimation("Dig", 5, Anim_Linear(0, 0, GetAnimationLength("Dig"), 36, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

	// Update carried items
	UpdateAttach();

	// Sound
	Sound("Dig*");

	// Set proper turn type
	SetTurnType(0);
}

func FxIntDigTimer(pTarget, effect, iTime)
{
	if(iTime % 36 == 0)
	{
		Sound("Dig*");
	}
	if( (iTime-18) % 36 == 0 ||  iTime > 35)
	{
		var noDig = 1;
		for(var pShovel in FindObjects(Find_ID(Shovel), Find_Container(this)))
			if(pShovel->IsDigging()) noDig = 0;
		if(noDig)
		{
			SetAction("Walk");
			SetComDir(COMD_Stop);
			return -1;
		}
	}
}

// custom throw
public func ControlThrow(object target, int x, int y)
{
	// standard throw after all
	if (!x && !y) return false;
	if (!target) return false;

	var throwAngle = Angle(0,0,x,y);

	// walking (later with animation: flight, scale, hangle?) and hands free
	if ( (GetProcedure() == "WALK" || GetAction() == "Jump" || GetAction() == "Dive")
		&& this->~HasHandAction())
	{
		if (throwAngle < 180) SetDir(DIR_Right);
		else SetDir(DIR_Left);
		//SetAction("Throw");
		this->~SetHandAction(1); // Set hands ocupied
		AddEffect("IntThrow", this, 1, 1, this, 0, target, throwAngle);
		return true;
	}
	// attached
	if (GetProcedure() == "ATTACH")
	{
		//SetAction("RideThrow");
		return DoThrow(target,throwAngle);
	}
	return false;
}

func FxIntThrowStart(target, effect, tmp, targetobj, throwAngle)
{
	var iThrowTime = 16;
	if(tmp) return;
	PlayAnimation("ThrowArms", 10, Anim_Linear(0, 0, GetAnimationLength("ThrowArms"), iThrowTime), Anim_Const(1000));
	effect.var0 = targetobj;
	effect.var1 = throwAngle;
}

func FxIntThrowTimer(target, effect, time)
{
	// cancel throw if object does not exist anymore
	if(!effect.var0)
		return -1;
	var iThrowTime = 16;
	if(time == iThrowTime*8/15)
		DoThrow(effect.var0, effect.var1);
	if(time >= iThrowTime)
		return -1;
}

func FxIntThrowStop(target, effect, reason, tmp)
{
	if(tmp) return;
	StopAnimation(GetRootAnimation(10));
	this->~SetHandAction(0);
}
func StartDead()
{
	PlayAnimation("Dead", 5, Anim_Linear(0, 0, GetAnimationLength("Dead"), 20, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(1);
}

func StartTumble()
{
	if(GetEffect("IntTumble", this)) return;
	// Close eyes
	CloseEyes(1);
	PlayAnimation("Tumble", 5, Anim_Linear(0, 0, GetAnimationLength("Tumble"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(0);
	AddEffect("IntTumble", this, 1, 0);
}

func StopTumble()
{
	if(GetAction() != "Tumble")
	{
		RemoveEffect("IntTumble", this);
		CloseEyes(-1);
	}
}

/* Riding */
public func StartRiding()
{
	if(!GetEffect("IntRiding", this))
		AddEffect("IntRiding", this, 1, 0, this);
}

public func AttachTargetLost()
{
	if(GetEffect("IntRiding", this))
		RemoveEffect("IntRiding", this);
}

public func StopRiding()
{
	if(GetEffect("IntRiding", this))
		RemoveEffect("IntRiding", this);
}

func FxIntRidingStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	var pMount = GetActionTarget();
	if(!pMount) return -1;
	if(pMount->~OnMount(this)) // Notifiy the mount, that the clonk is mounted (it should take care, that the clonk get's attached!
	{
		// if mount has returned true we should be attached
		// So make the clonk object invisible
		effect.var0 = GetProperty("Visibility");
		SetProperty("Visibility", VIS_None);
	}
	else effect.var0 = -1;
	effect.var1 = pMount;
}

func FxIntRidingStop(pTarget, effect, fTmp)
{
	if(fTmp) return;
	if(effect.var0 != -1)
		SetProperty("Visibility", effect.var0);

	var pMount = effect.var1;
	if(pMount)
		pMount->~OnUnmount(this);
}

// calback from engine
func OnMaterialChanged(int new, int old)
{
	if(!GetAlive()) return;
	var newdens = GetMaterialVal("Density","Material",new);
	var olddens = GetMaterialVal("Density","Material",old);
	var newliquid = (newdens >= C4M_Liquid) && (newdens < C4M_Solid);
	var oldliquid = (olddens >= C4M_Liquid) && (olddens < C4M_Solid);
	// into water
	if(newliquid && !oldliquid)
		AddEffect("Bubble", this, 1, 72, this);
	// out of water
	else if(!newliquid && oldliquid)
		RemoveEffect("Bubble", this);
}

func FxBubbleTimer(pTarget, effect, iTime)
{
	if(GBackLiquid(0,-5)) Bubble();
}

func StartPushing()
{
//	if(GetEffect("IntTumble", this)) return;
	// Close eyes
	PlayAnimation("Push", 5, Anim_AbsX(0, 0, GetAnimationLength("Push"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(1);
//	AddEffect("IntTumble", this, 1, 0);
}

protected func StopPushing()
{
	return _inherited(...);
}

func StartHangOnto()
{
//	if(GetEffect("IntTumble", this)) return;
	// Close eyes
	PlayAnimation("OnRope", 5, Anim_Linear(0, 0, GetAnimationLength("OnRope"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(1);
//	AddEffect("IntTumble", this, 1, 0);
}

protected func AbortHangOnto()
{
	if (GetActionTarget(0))
		GetActionTarget(0)->~HangOntoLost(this);
	return;
}

func QueryCatchBlow(object obj)
{
	var r=0;
	var e=0;
	var i=0;
	while(e=GetEffect("*Control*", this, i++))
	{
		if(EffectCall(this, e, "QueryCatchBlow", obj))
		{
			r=true;
			break;
		}
		
	}
	if(r) return r;
	return _inherited(obj, ...);
}

/* Act Map */

local ActMap = {
Walk = {
	Prototype = Action,
	Name = "Walk",
	Procedure = DFA_WALK,
	Accel = 16,
	Decel = 22,
	Speed = 196,
	Directions = 2,
	FlipDir = 0,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartWalk",
	AbortCall = "StopWalk",
//	InLiquidAction = "Swim",
},
Stand = {
	Prototype = Action,
	Name = "Stand",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartStand",
	InLiquidAction = "Swim",
},
Kneel = {
	Prototype = Action,
	Name = "Kneel",
	Procedure = DFA_KNEEL,
	Directions = 2,
	FlipDir = 0,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 8,
	Hgt = 20,
//	StartCall = "StartKneel",
	InLiquidAction = "Swim",
},
Scale = {
	Prototype = Action,
	Name = "Scale",
	Procedure = DFA_SCALE,
	Speed = 60,
	Accel = 20,
	Attach = CNAT_MultiAttach,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 20,
	Wdt = 8,
	Hgt = 20,
	OffX = 0,
	OffY = 0,
	StartCall = "StartScale",
	AbortCall = "StopScale",
},
Tumble = {
	Prototype = Action,
	Name = "Tumble",
	Procedure = DFA_FLIGHT,
	Speed = 200,
	Accel = 16,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 40,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Tumble",
	ObjectDisabled = 1,
	InLiquidAction = "Swim",
	StartCall = "StartTumble",
	AbortCall = "StopTumble",
	EndCall = "CheckStuck",
},
Dig = {
	Prototype = Action,
	Name = "Dig",
	Procedure = DFA_DIG,
	Speed = 50,
	Directions = 2,
	Length = 16,
	Delay = 0,//15*3*0,
	X = 0,
	Y = 60,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Dig",
	StartCall = "StartDigging",
	AbortCall = "StopDigging",
	DigFree = 11,
//	InLiquidAction = "Swim",
	Attach = CNAT_Left | CNAT_Right | CNAT_Bottom,
},
Bridge = {
	Prototype = Action,
	Name = "Bridge",
	Procedure = DFA_THROW,
	Directions = 2,
	Length = 16,
	Delay = 1,
	X = 0,
	Y = 60,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Bridge",
	InLiquidAction = "Swim",
},
Swim = {
	Prototype = Action,
	Name = "Swim",
	Procedure = DFA_SWIM,
	Speed = 96,
	Accel = 7,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 80,
	Wdt = 8,
	Hgt = 20,
	OffX = 0,
	OffY = 0,
//	SwimOffset = -5,
	StartCall = "StartSwim",
	AbortCall = "StopSwim",
},
Hangle = {
	Prototype = Action,
	Name = "Hangle",
	Procedure = DFA_HANGLE,
	Speed = 48,
	Accel = 20,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 100,
	Wdt = 8,
	Hgt = 20,
	OffX = 0,
	OffY = 3,
	StartCall = "StartHangle",
	AbortCall = "StopHangle",
	InLiquidAction = "Swim",
},
Jump = {
	Prototype = Action,
	Name = "Jump",
	Procedure = DFA_FLIGHT,
	Speed = 200,
	Accel = 16,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 120,
	Wdt = 8,
	Hgt = 20,
	InLiquidAction = "Swim",
	PhaseCall = "CheckStuck",
//	Animation = "Jump",
	StartCall = "StartJump",
},
Dive = {
	Prototype = Action,
	Name = "Dive",
	Procedure = DFA_FLIGHT,
	Speed = 200,
	Accel = 16,
	Directions = 2,
	Length = 8,
	Delay = 4,
	X = 0,
	Y = 160,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Hold",
	ObjectDisabled = 1,
	InLiquidAction = "Swim",
	PhaseCall = "CheckStuck",
},
Dead = {
	Prototype = Action,
	Name = "Dead",
	Directions = 2,
	X = 0,
	Y = 240,
	Wdt = 8,
	Hgt = 20,
	Length = 1,
	Delay = 0,
	NextAction = "Hold",
	StartCall = "StartDead",
	NoOtherAction = 1,
	ObjectDisabled = 1,
},
Ride = {
	Prototype = Action,
	Name = "Ride",
	Procedure = DFA_ATTACH,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 128,
	Y = 120,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartRiding",
	AbortCall = "StopRiding",
	InLiquidAction = "Swim",
},
Push = {
	Prototype = Action,
	Name = "Push",
	Procedure = DFA_PUSH,
	Speed = 196,
	Accel = 100,
	Directions = 2,
	Length = 8,
	Delay = 15,
	X = 128,
	Y = 140,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Push",
	StartCall = "StartPushing",
	AbortCall = "StopPushing",
	InLiquidAction = "Swim",
},
Build = {
	Prototype = Action,
	Name = "Build",
	Procedure = DFA_BUILD,
	Directions = 2,
	Length = 8,
	Delay = 15,
	X = 128,
	Y = 140,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Build",
	InLiquidAction = "Swim",
},
HangOnto = {
	Prototype = Action,
	Name = "HangOnto",
	Procedure = DFA_ATTACH,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 128,
	Y = 120,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartHangOnto",
	AbortCall = "AbortHangOnto",
	InLiquidAction = "Swim",
},
};
local Name = "Clonk";
local MaxEnergy = 50000;
local MaxBreath = 252; // Clonk can breathe for 7 seconds under water.
local JumpSpeed = 400;
local ThrowSpeed = 294;

func Definition(def) {
	// Set perspective
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,1000,5000), Trans_Rotate(70,0,1,0)), def);

	_inherited(def);
}
