#include "EnemyRobot.h"

EnemyRobot::~EnemyRobot()
{
	if (mDelegate)
	{
		mDelegate = NULL;
	}
	if (robotWalkingAnim)
	{
		robotWalkingAnim->release();
		robotWalkingAnim = NULL;
	}
	if (raisePhaserAnim)
	{
		raisePhaserAnim->release();
		raisePhaserAnim = NULL;
	}
	if (shootPhaserAnim)
	{
		shootPhaserAnim->release();
		shootPhaserAnim = NULL;
	}
	if (lowerPhaserAnim)
	{
		lowerPhaserAnim->release();
		lowerPhaserAnim = NULL;
	}
	if (torsoHitAnim)
	{
		torsoHitAnim->release();
		torsoHitAnim = NULL;
	}
	if (headHitAnim)
	{
		headHitAnim->release();
		headHitAnim = NULL;
	}
	if (robotDeathAnim)
	{
		robotDeathAnim->release();
		robotDeathAnim = NULL;
	}

#if ENEMY_STATE_DEBUG == 1
	if (myDebugLabel)
	{
		myDebugLabel->release();
		myDebugLabel = NULL;
	}
#endif
}

void EnemyRobot::setDelegate(GameplayLayerDelegate *pDelegate)
{
	mDelegate = pDelegate;
}

void EnemyRobot::shootPhaser()
{
	CCPoint phaserFiringPosition;
	PhaserDirection phaserDir;
	CCRect boundingBox = this->boundingBox();
	CCPoint position = this->getPosition();

	float xPosition = position.x + boundingBox.size.width * 0.542f;
	float yPosition = position.y + boundingBox.size.height * 0.25f;

	if (this->isFlipX()) 
	{
		//CCLOG("Facing right, Firing to the right");
		phaserDir = kDirectionRight;
	} 
	else 
	{
		//CCLOG("Facing left, Firing to the left");
		xPosition = position.x - boundingBox.size.width * 0.542f; // Reverse direction, this is the right way of doing it, the book is wrong
		phaserDir = kDirectionLeft;
	}
	phaserFiringPosition = ccp(xPosition, yPosition);
	mDelegate->createPhaserWithDirection(phaserDir, phaserFiringPosition);
}

CCRect EnemyRobot::eyesightBoundingBox()
{
	// Eyesight is 3 robot widths in the direction the robot is facing.
	CCRect robotSightBoundingBox;
	CCRect robotBoundingBox = this->adjustedBoundingBox();	
	if (this->isFlipX()) 
		robotSightBoundingBox = CCRectMake(robotBoundingBox.origin.x, robotBoundingBox.origin.y, robotBoundingBox.size.width*3.0f, robotBoundingBox.size.height);
	else 
		robotSightBoundingBox = CCRectMake(robotBoundingBox.origin.x - robotBoundingBox.size.width*2.0f, robotBoundingBox.origin.y, robotBoundingBox.size.width*3.0f, robotBoundingBox.size.height);
	return robotSightBoundingBox;
}

void EnemyRobot::changeState(CharacterStates newState)
{
	if (characterState == kStateDead)
		return; // No need to change state further once I am dead
	this->stopAllActions();
	STOPSOUNDEFFECT(walkingSound);
	CCFiniteTimeAction *action = NULL;
	characterState = newState;

	switch(newState)
	{
	case kStateSpawning:
		PLAYSOUNDEFFECT(ENEMYROBOT_TELEPORT);
		this->runAction(CCFadeOut::actionWithDuration(0.0f));
		// Fades out the sprite if it was visible before
		this->setDisplayFrame(CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName("teleport.png"));
		action = CCSpawn::actions(CCRotateBy::actionWithDuration(1.5f, 360), CCFadeIn::actionWithDuration(1.5f), NULL);
		break;
	
	case kStateIdle:
		//CCLOG("EnemyRobot->Changing State to Idle");
		this->setDisplayFrame(CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName("an1_anim1.png"));
		break;

	case kStateWalking:
	{
		//CCLOG("EnemyRobot->Changing State to Walking");
		if (isVikingWithinBoundingBox)
			break; // AI will change to Attacking on next frame

		this->playWalkingSound();

		float xPositionOffSet = 150.0f;
		if (isVikingWithinSight) 
		{
			if (vikingCharacter->getPosition().x < this->getPosition().x)
				xPositionOffSet = xPositionOffSet * -1;
				// Invert to -150
		} 
		else 
		{
			if (CCRANDOM_0_1() > 0.5f)
				xPositionOffSet = xPositionOffSet * -1;

			if (xPositionOffSet > 0.0f) 
				this->setFlipX(true);
		
			else 
				this->setFlipX(false);
		}
		action = CCSpawn::actions(CCAnimate::actionWithAnimation(robotWalkingAnim, false), CCMoveTo::actionWithDuration(2.4f, ccp(this->getPosition().x + xPositionOffSet, this->getPosition().y)), NULL); 
	
		break;
	}
	case kStateAttacking:
		//CCLOG("EnemyRobot->Changing State to Attacking");
		action = CCSequence::actions(CCAnimate::actionWithAnimation(raisePhaserAnim, false), 
									CCDelayTime::actionWithDuration(1.0f), 
									CCAnimate::actionWithAnimation(shootPhaserAnim, false), 
									CCCallFunc::actionWithTarget(this, callfunc_selector(EnemyRobot::shootPhaser)), 
									CCCallFunc::actionWithTarget(this, callfunc_selector(EnemyRobot::playPhaserFireSound)),
									CCAnimate::actionWithAnimation(lowerPhaserAnim, false), CCDelayTime::actionWithDuration(2.0f), 
									NULL);
		break;

	case kStateTakingDamage:
		//CCLOG("EnemyRobot->Changing State to TakingDamage");
		PLAYSOUNDEFFECT(ENEMYROBOT_DAMAGE);
		if (vikingCharacter->getWeaponDamage() > kVikingFistDamage)
		{
		// If the viking has the mallet, then
		action = CCAnimate::actionWithAnimation(headHitAnim, true);
		} 
		else 
		{
		// Viking does not have weapon, body blow
		action = CCAnimate::actionWithAnimation(torsoHitAnim, true);
		}
		break;

	case kStateDead:
		//CCLOG("EnemyRobot -> Going to Dead State");
		PLAYSOUNDEFFECT(ENEMYROBOT_DYING);
		action = CCSequence::actions(CCAnimate::actionWithAnimation(robotDeathAnim, false), CCDelayTime::actionWithDuration(2.0f), CCFadeOut::actionWithDuration(2.0f), NULL);
		break;

	default:
		//CCLOG("Enemy Robot -> Unknown CharState %d", characterState);
		break;
	}

	if (action != NULL)
		this->runAction(action);
}

void EnemyRobot::updateStateWithDeltaTime(ccTime deltaTime, CCArray *listOfGameObjects)
{
	this->checkAndClampSpritePosition();
	
#if ENEMY_STATE_DEBUG == 1
	this->setDebugLabelTextAndPosition();
#endif

	if ((characterState != kStateDead) && (characterHealth <= 0)) 
	{
		this->changeState(kStateDead);
		return;
	}

	vikingCharacter = (GameCharacter*) this->getParent()->getChildByTag(kVikingSpriteTagValue);
	CCRect vikingBoundingBox = vikingCharacter->adjustedBoundingBox();
	CCRect robotBoundingBox = this->adjustedBoundingBox();
	CCRect robotSightBoundingBox = this->eyesightBoundingBox();

	isVikingWithinBoundingBox = CCRect::CCRectIntersectsRect(vikingBoundingBox, robotBoundingBox) ? true : false;
	isVikingWithinSight = CCRect::CCRectIntersectsRect(vikingBoundingBox, robotSightBoundingBox)? true : false;
	// != kStateSpawning, so the enemy robot can't get hit while spawning
	if ((isVikingWithinBoundingBox) && (vikingCharacter->getCharacterState() == kStateAttacking && characterState != kStateSpawning)) 
	{
		// Viking is attacking this robot
		if ((characterState != kStateTakingDamage) && (characterState != kStateDead)) 
		{
			this->setCharacterHealth(this->getCharacterHealth() - vikingCharacter->getWeaponDamage());
			if (characterHealth > 0)
				this->changeState(kStateTakingDamage);
			else 
			{
				this->changeState(kStateDead);
				GameManager::sharedGameManager()->setKillCount(GameManager::sharedGameManager()->getKillCount() + 1);
			}

			return; // Nothing to update further, stop and show damage
		}
	}

	// This if statement is not in the book, but added so that the enemy robot starts shooting as soon as the viking is within sight
	// and not until the enemy robot finishes walking
	if (isVikingWithinSight && !isVikingWithinBoundingBox && vikingCharacter->getCharacterState() != kStateDead && characterState != kStateSpawning)
	{	
		if (characterState != kStateAttacking)
			this->changeState(kStateAttacking);
	}

	if (this->numberOfRunningActions() == 0) 
	{
		if (characterState == kStateDead) 
		{
			// Robot is dead, remove
			this->setIsVisible(false);
			this->removeFromParentAndCleanup(true);
		} 
		else if (vikingCharacter->getCharacterState() == kStateDead) 
		{
			// Viking is dead, walk around the scene
			this->changeState(kStateWalking);
		} 
		else if (isVikingWithinSight) 
		{
			this->changeState(kStateAttacking);
		} 
		else 
		{
			// Viking alive and out of sight, resume walking
			this->changeState(kStateWalking);
		}
	}
}

CCRect EnemyRobot::adjustedBoundingBox()
{
	// Shrink the bounding box by 18% on the X axis, and move it to the
	// right by 18% and crop it by 5% on the Y Axis.
	// On the iPad this is 30 pixels on the X axis and
	// 10 pixels from the top (Y Axis)
	CCRect enemyRobotBoundingBox = this->boundingBox();
	float xOffsetAmount = enemyRobotBoundingBox.size.width * 0.18f;
	float yCropAmount = enemyRobotBoundingBox.size.height * 0.05f;
	enemyRobotBoundingBox = CCRectMake(enemyRobotBoundingBox.origin.x + xOffsetAmount, enemyRobotBoundingBox.origin.y, enemyRobotBoundingBox.size.width - xOffsetAmount, enemyRobotBoundingBox.size.height - yCropAmount);
	return enemyRobotBoundingBox;
}

void EnemyRobot::initAnimations()
{
	this->setRobotWalkingAnim(this->loadPlistForAnimationWithName("robotWalkingAnim", "Plists/EnemyRobot.plist"));
	robotWalkingAnim->retain();
	this->setRaisePhaserAnim(this->loadPlistForAnimationWithName("raisePhaserAnim", "Plists/EnemyRobot.plist"));
	raisePhaserAnim->retain();
	this->setShootPhaserAnim(this->loadPlistForAnimationWithName("shootPhaserAnim", "Plists/EnemyRobot.plist"));
	shootPhaserAnim->retain();
	this->setLowerPhaserAnim(this->loadPlistForAnimationWithName("lowerPhaserAnim", "Plists/EnemyRobot.plist"));
	lowerPhaserAnim->retain();	
	this->setTorsoHitAnim(this->loadPlistForAnimationWithName("torsoHitAnim", "Plists/EnemyRobot.plist"));
	torsoHitAnim->retain();
	this->setHeadHitAnim(this->loadPlistForAnimationWithName("headHitAnim", "Plists/EnemyRobot.plist"));
	headHitAnim->retain();	
	this->setRobotDeathAnim(this->loadPlistForAnimationWithName("robotDeathAnim", "Plists/EnemyRobot.plist"));
	robotDeathAnim->retain();
}

void EnemyRobot::playPhaserFireSound()
{
	int soundToPlay = rand() % 2;

	if (soundToPlay == 0) 
		PLAYSOUNDEFFECT(ENEMYROBOT_PHASERFIRE_1);
	else 
		PLAYSOUNDEFFECT(ENEMYROBOT_PHASERFIRE_2);
}

void EnemyRobot::playWalkingSound() 
{
	int soundToPlay = rand() % 2;

	if (soundToPlay == 0) {
		walkingSound = PLAYSOUNDEFFECT(ENEMYROBOT_WALKING_1);
	} else {
		walkingSound = PLAYSOUNDEFFECT(ENEMYROBOT_WALKING_2);
	}
}

bool EnemyRobot::init()
{
	bool pRet = false;
	if (GameObject::init())
	{
		isVikingWithinBoundingBox = false;
		isVikingWithinSight = false;
		gameObjectType = kEnemyTypeAlienRobot;
		this->initAnimations();
		srand(time(NULL));
		pRet = true;
	}
	return pRet;
}

#if ENEMY_STATE_DEBUG == 1
void EnemyRobot::setDebugLabelTextAndPosition()
{
	CCPoint newPosition = this->getPosition();
	char labelString[100] = {0};
	sprintf(labelString, "X: %.2f \nY:%.2f \n", newPosition.x, newPosition.y);

	switch (characterState)
	{
		case kStateSpawning:
			myDebugLabel->setString(strcat(labelString, "Spawning"));
			break;
		case kStateIdle:
			myDebugLabel->setString(strcat(labelString, "Idle"));
			break;
		case kStateWalking:
			myDebugLabel->setString(strcat(labelString, "Walking"));
			break;
		case kStateAttacking:
			myDebugLabel->setString(strcat(labelString, "Attacking"));
			break;
		case kStateTakingDamage:
			myDebugLabel->setString(strcat(labelString, "Taking Damage"));
			break;
		case kStateDead:
			myDebugLabel->setString(strcat(labelString, "Dead"));
			break;
		default:
			myDebugLabel->setString(strcat(labelString, "Unknown State"));
			break;
	}

	//Retain otherwise it will cause an Access Violating Reading Location
	myDebugLabel->retain();
	float yOffset = SCREEN_HEIGHT * 0.195f;
	newPosition = ccp(newPosition.x, newPosition.y + yOffset);
	myDebugLabel->setPosition(newPosition);
}
#endif