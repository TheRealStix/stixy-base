#include "../Board.h"
#include "../Zombie.h"
#include "GameButton.h"
#include "StoreScreen.h"
#include "../Cutscene.h"
#include "../SeedPacket.h"
#include "../../LawnApp.h"
#include "AlmanacDialog.h"
#include "../System/Music.h"
#include "../../Resources.h"
#include "../../Lawn/Plant.h"
#include "../ToolTipWidget.h"
#include "SeedChooserScreen.h"
#include "../../GameConstants.h"
#include "../System/PlayerInfo.h"
#include "../System/PopDRMComm.h"
#include "../../SexyAppFramework/Debug.h"
#include "../../SexyAppFramework/Dialog.h"
#include "../../SexyAppFramework/MTRand.h"
#include "../../Sexy.TodLib/TodStringFile.h"
#include "../../SexyAppFramework/WidgetManager.h"
#include "../../SexyAppFramework/Slider.h"
#include "../System/PoolEffect.h"

const Rect cSeedClipRect = Rect(10, 294, 445, 252 + SEED_CHOOSER_EXTRA_HEIGHT);
const int cSeedPacketYOffset = 2;
const int cSeedPacketRows = 8;

SeedChooserScreen::SeedChooserScreen()
{
	mApp = (LawnApp*)gSexyAppBase;
	mBoard = mApp->mBoard;
	mClip = false;
	mSeedsInFlight = 0;
	mSeedsInBank = 0;
	mLastMouseX = -1;
	mLastMouseY = -1;
	mChooseState = CHOOSE_NORMAL;
	mViewLawnTime = 0;
	mToolTip = new ToolTipWidget();
	mToolTipSeed = -1;
	mScrollAmount = 0;
	mScrollPosition = 0;

	mOrderedSeedsDirty = true;
	RebuildOrderedSeeds();
	mPreviewSeed = mOrderedSeeds[0];
	mPlantPreview = nullptr;
	mPreviewSkin = mApp->mPlayerInfo->mSeedsSkin[(int)mPreviewSeed];

	mStartButton = new GameButton(SeedChooserScreen::SeedChooserScreen_Start);
	mStartButton->mLabel = _S("[LETS_ROCK_BUTTON]");
	mStartButton->mButtonImage = Sexy::IMAGE_SEEDCHOOSER_BUTTON;
	mStartButton->mOverImage = nullptr;
	mStartButton->mDownImage = nullptr;
	mStartButton->mDisabledImage = Sexy::IMAGE_SEEDCHOOSER_BUTTON_DISABLED;
	mStartButton->mOverOverlayImage = Sexy::IMAGE_SEEDCHOOSER_BUTTON_GLOW;
	mStartButton->SetFont(Sexy::FONT_DWARVENTODCRAFT18YELLOW);
	mStartButton->mColors[ButtonWidget::COLOR_LABEL] = Color::White;
	mStartButton->mColors[ButtonWidget::COLOR_LABEL_HILITE] = Color::White;
	mStartButton->Resize(154, 552 + SEED_CHOOSER_EXTRA_HEIGHT, 156, 42);
	mStartButton->mTextOffsetY = -1;
	EnableStartButton(false);
	mStartButton->mParentWidget = this;

	mFavButton = new GameButton(SeedChooserScreen::SeedChooserScreen_Fav);
	mFavButton->mButtonImage = !mApp->mPlayerInfo->mFavoriteSeeds[mPreviewSeed] ? Sexy::IMAGE_SEEDCHOOSER_BUTTON_FAV : Sexy::IMAGE_SEEDCHOOSER_BUTTON_FAV_ACTIVE;
	mFavButton->mOverImage = nullptr;
	mFavButton->mDownImage = nullptr;
	mFavButton->mDisabledImage = Sexy::IMAGE_SEEDCHOOSER_BUTTON_FAV_DISABLED;
	mFavButton->mOverOverlayImage = Sexy::IMAGE_SEEDCHOOSER_BUTTON_GLOW_SMALL;
	mFavButton->Resize(258, 108 + SEED_CHOOSER_EXTRA_HEIGHT, 40, 42);
	mFavButton->mParentWidget = this;

	mSkinButton = new GameButton(SeedChooserScreen::SeedChooserScreen_Skin);
	mSkinButton->mButtonImage = Sexy::IMAGE_SEEDCHOOSER_BUTTON_SKIN;
	mSkinButton->mOverImage = nullptr;
	mSkinButton->mDownImage = nullptr;
	mSkinButton->mDisabledImage = Sexy::IMAGE_SEEDCHOOSER_BUTTON_SKIN_DISABLED;
	mSkinButton->mOverOverlayImage = Sexy::IMAGE_SEEDCHOOSER_BUTTON_GLOW_SMALL;
	mSkinButton->Resize(302, 108 + SEED_CHOOSER_EXTRA_HEIGHT, 40, 42);
	mSkinButton->mParentWidget = this;

	mStatsButton = new GameButton(SeedChooserScreen::SeedChooserScreen_Stats);
	mStatsButton->mButtonImage = !mApp->mPlayerInfo->mShowStats ? Sexy::IMAGE_SEEDCHOOSER_BUTTON_STAT : Sexy::IMAGE_SEEDCHOOSER_BUTTON_STAT_ACTIVE;
	mStatsButton->mOverImage = nullptr;
	mStatsButton->mDownImage = nullptr;
	mStatsButton->mOverOverlayImage = Sexy::IMAGE_SEEDCHOOSER_BUTTON_GLOW_SMALL;
	mStatsButton->Resize(346, 108 + SEED_CHOOSER_EXTRA_HEIGHT, 40, 42);
	mStatsButton->mParentWidget = this;

	int aButtonOffsetX = BOARD_ADDITIONAL_WIDTH * 2;
	mMenuButton = new GameButton(SeedChooserScreen::SeedChooserScreen_Menu);
	mMenuButton->mLabel = _S("[MENU_BUTTON]");
	mMenuButton->mDrawStoneButton = true;
	mMenuButton->Resize(681 + aButtonOffsetX, 1, 117, 46);
	mMenuButton->mParentWidget = this;

	mRandomButton = new GameButton(SeedChooserScreen::SeedChooserScreen_Random);
	mRandomButton->mLabel = _S("[DEBUG_PLAY_BUTTON]");
	mRandomButton->mButtonImage = Sexy::IMAGE_BLANK;
	mRandomButton->mOverImage = Sexy::IMAGE_BLANK;
	mRandomButton->mDownImage = Sexy::IMAGE_BLANK;
	mRandomButton->SetFont(Sexy::FONT_BRIANNETOD12);
	mRandomButton->mColors[0] = Color(255, 240, 0);
	mRandomButton->mColors[1] = Color(200, 200, 255);
	mRandomButton->Resize(332, 555 + SEED_CHOOSER_EXTRA_HEIGHT, 100, 30);
	mRandomButton->mParentWidget = this;

	Color aBtnColor = Color(42, 42, 90);
	Image* aBtnImage = Sexy::IMAGE_SEEDCHOOSER_BUTTON2;
	Image* aOverImage = Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW;
	int aImageWidth = aBtnImage->GetWidth();
	int aImageHeight = aOverImage->GetHeight();

	mViewLawnButton = new GameButton(SeedChooserScreen::SeedChooserScreen_ViewLawn);
	mViewLawnButton->mLabel = _S("[VIEW_LAWN]");
	mViewLawnButton->mButtonImage = aBtnImage;
	mViewLawnButton->mOverImage = aOverImage;
	mViewLawnButton->mDownImage = nullptr;
	mViewLawnButton->SetFont(Sexy::FONT_BRIANNETOD12);
	mViewLawnButton->mColors[0] = aBtnColor;
	mViewLawnButton->mColors[1] = aBtnColor;
	mViewLawnButton->Resize(22, 561 + SEED_CHOOSER_EXTRA_HEIGHT, aImageWidth, aImageHeight);
	mViewLawnButton->mTextOffsetY = 1;
	mViewLawnButton->mParentWidget = this;
	if (!mBoard->mCutScene->IsSurvivalRepick())
	{
		mViewLawnButton->mBtnNoDraw = true;
		mViewLawnButton->mDisabled = true;
	}

	mAlmanacButton = new GameButton(SeedChooserScreen::SeedChooserScreen_Almanac);
	mAlmanacButton->mLabel = _S("[ALMANAC_BUTTON]");
	mAlmanacButton->mButtonImage = aBtnImage;
	mAlmanacButton->mOverImage = aOverImage;
	mAlmanacButton->mDownImage = nullptr;
	mAlmanacButton->SetFont(Sexy::FONT_BRIANNETOD12);
	mAlmanacButton->mColors[0] = aBtnColor;
	mAlmanacButton->mColors[1] = aBtnColor;
	mAlmanacButton->Resize(560 + aButtonOffsetX, 572 + SEED_CHOOSER_EXTRA_HEIGHT, aImageWidth, aImageHeight);
	mAlmanacButton->mTextOffsetY = 1;
	mAlmanacButton->mParentWidget = this;

	mStoreButton = new GameButton(SeedChooserScreen::SeedChooserScreen_Store);
	mStoreButton->mLabel = _S("[SHOP_BUTTON]");
	mStoreButton->mButtonImage = aBtnImage;
	mStoreButton->mOverImage = aOverImage;
	mStoreButton->mDownImage = nullptr;
	mStoreButton->SetFont(Sexy::FONT_BRIANNETOD12);
	mStoreButton->mColors[0] = aBtnColor;
	mStoreButton->mColors[1] = aBtnColor;
	mStoreButton->Resize(680 + aButtonOffsetX, 572 + SEED_CHOOSER_EXTRA_HEIGHT, aImageWidth, aImageHeight);
	mStoreButton->mTextOffsetY = 1;
	mStoreButton->mParentWidget = this;

	if (!mApp->CanShowAlmanac())
	{
		mAlmanacButton->mBtnNoDraw = true;
		mAlmanacButton->mDisabled = true;
	}
	if (!mApp->CanShowStore())
	{
		mStoreButton->mBtnNoDraw = true;
		mStoreButton->mDisabled = true;
	}
	if (mPreviewSeed == SEED_IMITATER) mFavButton->mDisabled = true;
	mSkinButton->mDisabled = !PlantHasSkin(mPreviewSeed, 1);

	DBG_ASSERT(mApp->GetSeedsAvailable() < NUM_SEED_TYPES);
	memset(mChosenSeeds, 0, sizeof(mChosenSeeds));
	for (SeedType aSeedType = SEED_PEASHOOTER; aSeedType < NUM_SEEDS_IN_CHOOSER; aSeedType = (SeedType)(aSeedType + 1))
	{
		ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
		aChosenSeed.mSeedType = aSeedType;
		GetSeedPositionInChooser(aSeedType, aChosenSeed.mX, aChosenSeed.mY);
		aChosenSeed.mTimeStartMotion = 0;
		aChosenSeed.mTimeEndMotion = 0;
		aChosenSeed.mStartX = aChosenSeed.mX;
		aChosenSeed.mStartY = aChosenSeed.mY;
		aChosenSeed.mEndX = aChosenSeed.mX;
		aChosenSeed.mEndY = aChosenSeed.mY;
		aChosenSeed.mSeedState = SEED_IN_CHOOSER;
		aChosenSeed.mSeedIndexInBank = 0;
		aChosenSeed.mRefreshCounter = 0;
		aChosenSeed.mRefreshing = false;
		aChosenSeed.mImitaterType = SEED_NONE;
		aChosenSeed.mCrazyDavePicked = false;
	}
	if (mBoard->mCutScene->IsSurvivalRepick())
	{
		for (int anIdx = 0; anIdx < mBoard->mSeedBank->mNumPackets; anIdx++)
		{
			SeedPacket* aSeedPacket = &mBoard->mSeedBank->mSeedPackets[anIdx];
			ChosenSeed& aChosenSeed = mChosenSeeds[aSeedPacket->mPacketType];
			aChosenSeed.mRefreshing = aSeedPacket->mRefreshing;
			aChosenSeed.mRefreshCounter = aSeedPacket->mRefreshCounter;
		}
		mBoard->mSeedBank->mNumPackets = 0;
	}
	if (mApp->mGameMode == GAMEMODE_CHALLENGE_SEEING_STARS)
	{
		ChosenSeed& aStarFruit = mChosenSeeds[SEED_STARFRUIT];
		int aX = mBoard->GetSeedPacketPositionX(0);
		aStarFruit.mX = aX, aStarFruit.mY = 8;
		aStarFruit.mStartX = aX, aStarFruit.mStartY = 8;
		aStarFruit.mEndX = aX, aStarFruit.mEndY = 8;
		aStarFruit.mSeedState = SEED_IN_BANK;
		aStarFruit.mSeedIndexInBank = 0;
		mSeedsInBank++;
	}
	if ((mApp->mCrazySeeds && mApp->mPlayingQuickplay) || (mApp->IsAdventureMode() && !mApp->IsFirstTimeAdventureMode() && !mApp->mPlayingQuickplay))
			CrazyDavePickSeeds();

	mSlider = new Sexy::Slider(IMAGE_OPTIONS_SLIDERSLOT_PLANT, IMAGE_OPTIONS_SLIDERKNOB_PLANT, 0, this);
	mSlider->SetValue(max(0.0, min(mMaxScrollPosition, mScrollPosition)));
	mSlider->mHorizontal = false;
	mSlider->mThumbOffsetX = -14;
	mSlider->mNoDraw = true;
	ResizeSlider();
	SetupPlantPreview();
}

int SeedChooserScreen::PickFromWeightedArrayUsingSpecialRandSeed(TodWeightedArray* theArray, int theCount, MTRand& theLevelRNG)
{
	int aTotalWeight = 0;
	for (int i = 0; i < theCount; i++)
		aTotalWeight += theArray[i].mWeight;
	DBG_ASSERT(aTotalWeight > 0);

	int aRndResult = theLevelRNG.Next((unsigned long)aTotalWeight);
	int aWeight = 0;
	for (int j = 0; j < theCount; j++)
	{
		aWeight += theArray[j].mWeight;
		if (aWeight > aRndResult) return theArray[j].mItem;
	}
	DBG_ASSERT(false);
}

void SeedChooserScreen::CrazyDavePickSeeds() //WIDETWEAK: Fixes the seed packets Crazy Dave chooses so you won't end up with Lily Pad on roof levels. By BeeTeeKay
{
	TodWeightedArray aSeedArray[NUM_SEED_TYPES];
	for (SeedType aSeedType = SEED_PEASHOOTER; aSeedType < NUM_SEEDS_IN_CHOOSER; aSeedType = (SeedType)(aSeedType + 1))
	{
		aSeedArray[aSeedType].mItem = aSeedType;
		uint aRecFlags = SeedNotRecommendedToPick(aSeedType);
		if ((aSeedType == SEED_GATLINGPEA && !mApp->mPlayerInfo->mPurchases[STORE_ITEM_PLANT_GATLINGPEA]) || !mApp->SeedTypeAvailable(aSeedType) ||
			SeedNotAllowedToPick(aSeedType) || Plant::IsUpgrade(aSeedType) || aSeedType == SEED_IMITATER || aSeedType == SEED_UMBRELLA ||
			aSeedType == SEED_BLOVER || aSeedType == SEED_GRAVEBUSTER || aSeedType == SEED_PLANTERN || Plant::IsNocturnal(aSeedType) ||
			aSeedType == SEED_FLOWERPOT || Plant::IsAquatic(aSeedType))
		{
			aSeedArray[aSeedType].mWeight = 0;
		}
		else
		{
			aSeedArray[aSeedType].mWeight = 1;
		}
		if (mBoard->StageIsNight()) {
			if (Plant::IsNocturnal(aSeedType) && !Plant::IsUpgrade(aSeedType) && !Plant::IsAquatic(aSeedType))
			{
				aSeedArray[aSeedType].mWeight = 1;
			}
		}
		if (mBoard->StageHasPool()) {
			if (Plant::IsAquatic(aSeedType) && !Plant::IsUpgrade(aSeedType) && !Plant::IsNocturnal(aSeedType))
			{
				aSeedArray[aSeedType].mWeight = 1;
			}
			else if (mBoard->StageIsNight() && Plant::IsAquatic(aSeedType) && Plant::IsNocturnal(aSeedType) && !Plant::IsUpgrade(aSeedType))
			{
				aSeedArray[aSeedType].mWeight = 1;
			}
		}
	}
	if (mBoard->StageIsNight())
	{
		aSeedArray[SEED_INSTANT_COFFEE].mWeight = 0;
	}
	if (mBoard->StageHasGraveStones())
	{
		aSeedArray[SEED_GRAVEBUSTER].mWeight = 1;
	}
	if (mBoard->StageHasFog())
	{
		aSeedArray[SEED_PLANTERN].mWeight = 1;
	}
	if (mBoard->mZombieAllowed[ZOMBIE_BALLOON] || mBoard->StageHasFog())
	{
		aSeedArray[SEED_BLOVER].mWeight = 1;
	}
	if (mBoard->StageHasRoof())
	{
		aSeedArray[SEED_TORCHWOOD].mWeight = 0;
		aSeedArray[SEED_SPIKEWEED].mWeight = 0;
		aSeedArray[SEED_FLOWERPOT].mWeight = 1;
	}
	if (mBoard->mZombieAllowed[ZOMBIE_BUNGEE] || mBoard->mZombieAllowed[ZOMBIE_CATAPULT])
	{
		aSeedArray[SEED_UMBRELLA].mWeight = 1;
	}

	MTRand aLevelRNG = MTRand(mBoard->GetLevelRandSeed());
	for (int i = 0; i < 3; i++)
	{
		SeedType aPickedSeed = (SeedType)PickFromWeightedArrayUsingSpecialRandSeed(aSeedArray, NUM_SEEDS_IN_CHOOSER, aLevelRNG);
		aSeedArray[aPickedSeed].mWeight = 0;
		ChosenSeed& aChosenSeed = mChosenSeeds[aPickedSeed];

		int aPosX = mBoard->GetSeedPacketPositionX(i);
		aChosenSeed.mX = aPosX;
		aChosenSeed.mY = 8;
		aChosenSeed.mStartX = aPosX;
		aChosenSeed.mStartY = 8;
		aChosenSeed.mEndX = aPosX;
		aChosenSeed.mEndY = 8;
		aChosenSeed.mSeedState = SEED_IN_BANK;
		aChosenSeed.mSeedIndexInBank = i;
		aChosenSeed.mCrazyDavePicked = true;
		mSeedsInBank++;
	}
}

bool SeedChooserScreen::Has7Rows()
{
	PlayerInfo* aPlayer = mApp->mPlayerInfo;
	if (mApp->HasFinishedAdventure() || mApp->mPlayerInfo->mPurchases[STORE_ITEM_PLANT_GATLINGPEA]) return true;
	for (SeedType aSeedType = SEED_TWINSUNFLOWER; aSeedType < SEED_COBCANNON; aSeedType = (SeedType)(aSeedType + 1))
		if (aSeedType != SEED_SPIKEROCK && mApp->SeedTypeAvailable(aSeedType)) return true;
	return false;
}

void SeedChooserScreen::GetSeedPositionInChooser(int theIndex, int& x, int& y)
{
	if (theIndex == SEED_IMITATER)
	{
		x = IMITATER_POS_X + 5;
		y = IMITATER_POS_Y + 12 + SEED_CHOOSER_EXTRA_HEIGHT;
	}
	else
	{
		x = theIndex % cSeedPacketRows * 53 + 22;
		y = theIndex / cSeedPacketRows * (SEED_PACKET_HEIGHT + cSeedPacketYOffset) + (SEED_PACKET_HEIGHT + 231) - mScrollPosition; //53  where seeds start
	}
}

void SeedChooserScreen::GetSeedPositionInBank(int theIndex, int& x, int& y)
{
	x = mBoard->mSeedBank->mX - mX + mBoard->GetSeedPacketPositionX(theIndex);
	y = mBoard->mSeedBank->mY - mY + 8;
}

SeedChooserScreen::~SeedChooserScreen()
{
	if (mStartButton) delete mStartButton;
	if (mFavButton) delete mFavButton;
	if (mSkinButton) delete mSkinButton;
	if (mStatsButton) delete mStatsButton;
	if (mRandomButton) delete mRandomButton;
	if (mViewLawnButton) delete mViewLawnButton;
	if (mAlmanacButton) delete mAlmanacButton;
	if (mStoreButton) delete mStoreButton;
	if (mSlider) delete mSlider;
	if (mToolTip) delete mToolTip;
	if (mMenuButton) delete mMenuButton;
	if (mPlantPreview) delete mPlantPreview;
}

void SeedChooserScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	RemoveWidget(mSlider);
}

void SeedChooserScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	AddWidget(mSlider);
}


unsigned int SeedChooserScreen::SeedNotRecommendedToPick(SeedType theSeedType)
{
	uint aRecFlags = mBoard->SeedNotRecommendedForLevel(theSeedType);
	if (TestBit(aRecFlags, NOT_RECOMMENDED_NOCTURNAL) && PickedPlantType(SEED_INSTANT_COFFEE))
		SetBit(aRecFlags, NOT_RECOMMENDED_NOCTURNAL, false);
	return aRecFlags;
}

bool SeedChooserScreen::SeedNotAllowedToPick(SeedType theSeedType)
{
	return mApp->mGameMode == GAMEMODE_CHALLENGE_LAST_STAND && (theSeedType == SEED_SUNFLOWER || theSeedType == SEED_SUNSHROOM ||
		theSeedType == SEED_TWINSUNFLOWER || theSeedType == SEED_SEASHROOM || theSeedType == SEED_PUFFSHROOM);
}

bool SeedChooserScreen::SeedNotAllowedDuringTrial(SeedType theSeedType)
{
	return mApp->IsTrialStageLocked() && (theSeedType == SEED_SQUASH || theSeedType == SEED_THREEPEATER);
}

void SeedChooserScreen::Draw(Graphics* g)
{
	if (mApp->GetDialog(DIALOG_STORE) || mApp->GetDialog(DIALOG_ALMANAC))
		return;

	g->SetLinearBlend(true);
	if (!mBoard->ChooseSeedsOnCurrentLevel() || (mBoard->mCutScene && mBoard->mCutScene->IsBeforePreloading()))
		return;

	g->DrawImage(Sexy::IMAGE_SEEDCHOOSER_BACKGROUND, 0, 87);
	if (mPreviewSeed == SeedType::SEED_LILYPAD || mPreviewSeed == SeedType::SEED_TANGLEKELP ||
		mPreviewSeed == SeedType::SEED_CATTAIL || mPreviewSeed == SeedType::SEED_SEASHROOM)
	{
		bool aNight = mPreviewSeed == SeedType::SEED_SEASHROOM;
		g->DrawImage(aNight ? Sexy::IMAGE_SEEDCHOOSER_GROUNDNIGHTPOOL : Sexy::IMAGE_SEEDCHOOSER_GROUNDPOOL, 37, 125);

		if (mApp->Is3dAccel())
		{
			g->SetClipRect(0, 0, 250, 380);
			g->mTransY -= 125;
			g->mTransX += 0;
			mApp->mPoolEffect->PoolEffectDraw(g, aNight);
			g->mTransY += 125;
			g->mTransX -= 0;
			g->ClearClipRect();
		}
	}
	else
	{
		g->DrawImage(
			Plant::IsNocturnal(mPreviewSeed) || mPreviewSeed == SeedType::SEED_GRAVEBUSTER || mPreviewSeed == SeedType::SEED_PLANTERN ? Sexy::IMAGE_SEEDCHOOSER_GROUNDNIGHT :
			mPreviewSeed == SeedType::SEED_FLOWERPOT ? Sexy::IMAGE_SEEDCHOOSER_GROUNDROOF : Sexy::IMAGE_SEEDCHOOSER_GROUNDDAY,
			37, 125);
	}
	if (mPlantPreview)
	{
		Graphics aPlantGraphics = Graphics(*g);
		mPlantPreview->BeginDraw(&aPlantGraphics);
		mPlantPreview->Draw(&aPlantGraphics);
	}
	g->DrawImage(Sexy::IMAGE_SEEDCHOOSER_PLANTCARD, 0, 125);
	if (mApp->SeedTypeAvailable(SEED_IMITATER))
		g->DrawImage(Sexy::IMAGE_SEEDCHOOSER_IMITATERADDON, IMITATER_POS_X, IMITATER_POS_Y + SEED_CHOOSER_EXTRA_HEIGHT);
	TodDrawString(g, _S("[CHOOSE_YOUR_PLANTS]"), 229, 110, Sexy::FONT_DWARVENTODCRAFT18YELLOW, Color::White, DS_ALIGN_CENTER);
	PlantDefinition& aPlantDef = GetPlantDefinition(mPreviewSeed);
	SexyString aName = Plant::GetNameString(mPreviewSeed, SEED_NONE);
	TodDrawString(g, aName, 136, 286, Sexy::FONT_DWARVENTODCRAFT18YELLOW, Color::White, DS_ALIGN_CENTER);

	Rect infoRect = Rect(266, 142, 155, 210);
	Font* font = Sexy::FONT_BRIANNETOD12;
	Color color = Color(40, 50, 90);

	if (mApp->mPlayerInfo->mShowStats && mPreviewSeed != SEED_IMITATER)
	{
		float rechargeSeconds = aPlantDef.mRefreshTime / 100.0f;
		SexyString value = fmod(rechargeSeconds, 1.0f) == 0.0f ? StrFormat(_S("%.0fs"), rechargeSeconds) : StrFormat(_S("%.1fs"), rechargeSeconds);
		SexyString statsText = StrFormat(_S("{KEYWORD}{WAIT_TIME}:{STAT} %s"), value.c_str());
		statsText = TodReplaceString(statsText, _S("{WAIT_TIME}"), _S("[WAIT_TIME]"));
		TodDrawStringWrapped(g, statsText, infoRect, Sexy::FONT_BRIANNETOD12, Color(40, 50, 90), DS_ALIGN_LEFT);
		int spacing = TodDrawStringWrappedHelper(g, statsText, infoRect, Sexy::FONT_BRIANNETOD12, Color(40, 50, 90), DS_ALIGN_LEFT, false);
		infoRect.mY += spacing;
		infoRect.mHeight -= spacing;
	}

	const char* desc = "[%s_TOOLTIP]";
	if (mApp->mPlayerInfo->mShowStats) desc = "[%s_STATS_ADVANCED]";
	TodDrawStringWrapped(g, StrFormat(_S(desc), aPlantDef.mPlantName), infoRect, Sexy::FONT_BRIANNETOD12, Color(40, 50, 90), DS_ALIGN_LEFT);
	
	mSlider->SliderDraw(g);
	mFavButton->Draw(g);
	mSkinButton->Draw(g);
	mStatsButton->Draw(g);

	if (mOrderedSeedsDirty) RebuildOrderedSeeds();
	for (int i = 0; i < mOrderedSeeds.size(); i++)
	{
		SeedType aSeedType = mOrderedSeeds[i];
		if (aSeedType != SEED_IMITATER)
			g->SetClipRect(cSeedClipRect);
		// Shadowed seeds in chooser
		int x, y;
		GetSeedPositionInChooser(i, x, y);

		if (mApp->SeedTypeAvailable(aSeedType))
		{
			ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
			if (aChosenSeed.mSeedState != SEED_IN_CHOOSER)
			{
				DrawSeedPacket(g, x, y, aSeedType, SEED_NONE, 0, 55, aSeedType != SEED_IMITATER, false);
			}
		}
		else if(aSeedType != SEED_IMITATER)
		{
			g->DrawImage(Sexy::IMAGE_SEEDPACKETSILHOUETTE, x, y);
		}

		// Regular seeds in chooser and bank
		ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
		ChosenSeedState aSeedState = aChosenSeed.mSeedState;
		if (mApp->SeedTypeAvailable(aSeedType) &&
			aSeedState != SEED_FLYING_TO_BANK &&
			aSeedState != SEED_FLYING_TO_CHOOSER &&
			aSeedState != SEED_PACKET_HIDDEN &&
			(aSeedState == SEED_IN_CHOOSER || mBoard->mCutScene->mSeedChoosing))
		{
			bool aGrayed = false;
			if (((SeedNotRecommendedToPick(aSeedType) || SeedNotAllowedToPick(aSeedType) || IsImitaterUnselectable(aSeedType)) && aSeedState == SEED_IN_CHOOSER) || SeedNotAllowedDuringTrial(aSeedType))
				aGrayed = true;

			int aPosX, aPosY;
			if (aSeedState == SEED_IN_CHOOSER)
			{
				GetSeedPositionInChooser((int)i, aPosX, aPosY);
			}
			else if (aSeedState == SEED_IN_BANK)
			{
				g->ClearClipRect();
				aPosX = aChosenSeed.mX;
				aPosY = aChosenSeed.mY;
			}
			else
			{
				aPosX = aChosenSeed.mX;
				aPosY = aChosenSeed.mY;
			}
			if (mChooseState != CHOOSE_VIEW_LAWN || (mChooseState == CHOOSE_VIEW_LAWN && aSeedState == SEED_IN_CHOOSER))
			{
				DrawSeedPacket(g, aPosX, aPosY, aChosenSeed.mSeedType, aChosenSeed.mImitaterType, 0, aGrayed ? 115 : 255, aSeedType != SEED_IMITATER || aSeedState != SEED_IN_CHOOSER, false);
			}
		}
		g->ClearClipRect();
	}

	// Draw flying seeds
	for (SeedType aSeedType = SEED_PEASHOOTER; aSeedType < NUM_SEEDS_IN_CHOOSER; aSeedType = (SeedType)(aSeedType + 1))
	{
		ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
		ChosenSeedState aSeedState = aChosenSeed.mSeedState;
		if (mApp->SeedTypeAvailable(aSeedType) && (aSeedState == SEED_FLYING_TO_BANK || aSeedState == SEED_FLYING_TO_CHOOSER))
		{
			DrawSeedPacket(g, aChosenSeed.mX, aChosenSeed.mY, aChosenSeed.mSeedType, aChosenSeed.mImitaterType, 0, 255, true, false);
		}
	}

	//mask
	g->SetClipRect(cSeedClipRect);
	int fadeHeight = 5;
	float t = mSlider->mVal;
	float topStrength = (t <= 0.0f) ? 0.0f : powf(t, 0.25f);
	float bottomStrength = (t >= 1.0f) ? 0.0f : powf(1.0f - t, 0.25f);
	for (int i = 0; i < fadeHeight; i++)
	{
		int baseAlphaTop = (int)((180 - i * 35) * topStrength);
		int baseAlphaBottom = (int)((180 - i * 35) * bottomStrength);
		int inset = fadeHeight - i - 1;
		int x = cSeedClipRect.mX + inset;
		int w = cSeedClipRect.mWidth - inset * 2;
		g->SetColor(Color(0, 0, 0, baseAlphaTop));
		g->FillRect(x,cSeedClipRect.mY + i,w,1);
		g->SetColor(Color(0, 0, 0, baseAlphaBottom));
		g->FillRect(x,cSeedClipRect.mY + cSeedClipRect.mHeight - 1 - i,w,1);
	}
	g->ClearClipRect();

	int aNumSeedsInBank = mBoard->mSeedBank->mNumPackets;
	for (int anIndex = 0; anIndex < aNumSeedsInBank; anIndex++)
	{
		if (FindSeedInBank(anIndex) == SEED_NONE)
		{
			int x, y;
			GetSeedPositionInBank(anIndex, x, y);
			g->DrawImage(Sexy::IMAGE_SEEDPACKETSILHOUETTE, x, y);
		}
	}

	mStartButton->Draw(g);
	mRandomButton->Draw(g);
	mViewLawnButton->Draw(g);
	mAlmanacButton->Draw(g);
	mStoreButton->Draw(g);
	Graphics aBoardFrameG = Graphics(*g);
	aBoardFrameG.mTransX -= mX;
	aBoardFrameG.mTransY -= mY;
	mMenuButton->Draw(&aBoardFrameG);
	mToolTip->Draw(g);
}

void SeedChooserScreen::UpdateViewLawn()
{
	if (mChooseState != CHOOSE_VIEW_LAWN) return;
	mViewLawnTime++;
	if (mViewLawnTime == 100) mBoard->DisplayAdviceAgain("[CLICK_TO_CONTINUE]", MESSAGE_STYLE_HINT_STAY, ADVICE_CLICK_TO_CONTINUE);
	else if (mViewLawnTime == 251) mViewLawnTime = 250;

	for (int anIndex = 0; anIndex < mBoard->mSeedBank->mNumPackets; anIndex++)
	{
		SeedType aSeedType = FindSeedInBank(anIndex);
		if (aSeedType == SEED_NONE) break;
		ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
		SeedPacket& aSeedPacket = mBoard->mSeedBank->mSeedPackets[anIndex];
		aSeedPacket.SetPacketType(aSeedType, aChosenSeed.mImitaterType);
	}

	int aSeedChooserY = SEED_CHOOSER_OFFSET_Y - Sexy::IMAGE_SEEDCHOOSER_BACKGROUND->GetHeight() - 87;
	int aStreetOffset = BOARD_IMAGE_WIDTH_OFFSET + BOARD_ADDITIONAL_WIDTH - mApp->mWidth;
	if (mViewLawnTime <= 100)
	{
		mBoard->mRoofPoleOffset = TodAnimateCurve(0, 100, mViewLawnTime, ROOF_POLE_END, ROOF_POLE_START, TodCurves::CURVE_EASE_IN_OUT);
		mBoard->mRoofTreeOffset = TodAnimateCurve(0, 100, mViewLawnTime, ROOF_TREE_END, ROOF_TREE_START, TodCurves::CURVE_EASE_IN_OUT);
		mBoard->Move(-TodAnimateCurve(0, 100, mViewLawnTime, aStreetOffset, 0, CURVE_EASE_IN_OUT), 0);
		Move(0, TodAnimateCurve(0, 40, mViewLawnTime, aSeedChooserY, SEED_CHOOSER_OFFSET_Y, CURVE_EASE_IN_OUT));
	}
	else if (mViewLawnTime <= 250)
	{
		mBoard->Move(0, 0);
		Move(0, SEED_CHOOSER_OFFSET_Y);
	}
	else if (mViewLawnTime <= 350)
	{
		mBoard->ClearAdvice(ADVICE_CLICK_TO_CONTINUE);
		mBoard->mRoofPoleOffset = TodAnimateCurve(250, 350, mViewLawnTime, ROOF_POLE_START, ROOF_POLE_END, TodCurves::CURVE_EASE_IN_OUT);
		mBoard->mRoofTreeOffset = TodAnimateCurve(250, 350, mViewLawnTime, ROOF_TREE_START, ROOF_TREE_END, TodCurves::CURVE_EASE_IN_OUT);
		mBoard->Move(-TodAnimateCurve(250, 350, mViewLawnTime, 0, aStreetOffset, CURVE_EASE_IN_OUT), 0);
		Move(0, TodAnimateCurve(310, 350, mViewLawnTime, SEED_CHOOSER_OFFSET_Y, aSeedChooserY, CURVE_EASE_IN_OUT));
	}
	else
	{
		mChooseState = CHOOSE_NORMAL;
		mViewLawnTime = 0;
		mMenuButton->mDisabled = false;

		for (int anIndex = 0; anIndex < mBoard->mSeedBank->mNumPackets; anIndex++)
		{
			SeedPacket& aSeedPacket = mBoard->mSeedBank->mSeedPackets[anIndex];
			aSeedPacket.SetPacketType(SEED_NONE, SEED_NONE);
		}
	}
}

void SeedChooserScreen::LandFlyingSeed(ChosenSeed& theChosenSeed)
{
	if (theChosenSeed.mSeedState == SEED_FLYING_TO_BANK)
	{
		theChosenSeed.mX = theChosenSeed.mEndX;
		theChosenSeed.mY = theChosenSeed.mEndY;
		theChosenSeed.mTimeStartMotion = 0;
		theChosenSeed.mTimeEndMotion = 0;
		theChosenSeed.mSeedState = SEED_IN_BANK;
		mSeedsInFlight--;
	}
	else if (theChosenSeed.mSeedState == SEED_FLYING_TO_CHOOSER)
	{
		theChosenSeed.mX = theChosenSeed.mEndX;
		theChosenSeed.mY = theChosenSeed.mEndY;
		theChosenSeed.mTimeStartMotion = 0;
		theChosenSeed.mTimeEndMotion = 0;
		theChosenSeed.mSeedState = SEED_IN_CHOOSER;
		mSeedsInFlight--;
		if (theChosenSeed.mSeedType == SEED_IMITATER)
		{
			theChosenSeed.mImitaterType = SEED_NONE;
		}
	}
}

void SeedChooserScreen::UpdateCursor()
{
	if (mApp->GetDialogCount() || mBoard->mCutScene->IsInShovelTutorial() || mApp->mGameMode == GAMEMODE_UPSELL || mSlider->mIsOver || mSlider->mDragging) return;
	SeedType aMouseSeedType = SeedHitTest(mLastMouseX, mLastMouseY);
	if (aMouseSeedType != SEED_NONE)
	{
		if (IsImitaterUnselectable(aMouseSeedType))
			aMouseSeedType = SEED_NONE;
		ChosenSeed& aMouseChosenSeed = mChosenSeeds[aMouseSeedType];
		if (aMouseChosenSeed.mSeedState == SEED_IN_BANK && aMouseChosenSeed.mCrazyDavePicked)
			aMouseSeedType = SEED_NONE;
	}
	if (mMouseVisible && mChooseState != CHOOSE_VIEW_LAWN && ((ZombieHitTest(mLastMouseX, mLastMouseY) && mApp->CanShowAlmanac() && !IsOverImitater(mLastMouseX, mLastMouseY)) || (aMouseSeedType != SEED_NONE && !SeedNotAllowedToPick(aMouseSeedType)) ||
		mRandomButton->IsMouseOver() || mViewLawnButton->IsMouseOver() || mAlmanacButton->IsMouseOver() ||
		mStoreButton->IsMouseOver() || mMenuButton->IsMouseOver() || mStartButton->IsMouseOver() ||
		mFavButton->IsMouseOver() || mSkinButton->IsMouseOver() || mStatsButton->IsMouseOver()))
		mApp->SetCursor(CURSOR_HAND);
	else
		mApp->SetCursor(CURSOR_POINTER);
}

void SeedChooserScreen::Update()
{
	Widget::Update();

	mRandomButton->mBtnNoDraw = !mApp->mTodCheatKeys;
	mRandomButton->mDisabled = !mApp->mTodCheatKeys;
	mMaxScrollPosition = max(0, (((NUM_SEEDS_IN_CHOOSER - 2) / cSeedPacketRows) * (SEED_PACKET_HEIGHT + cSeedPacketYOffset)) + SEED_PACKET_HEIGHT - cSeedClipRect.mHeight + 14);
	mSlider->mVisible = mMaxScrollPosition != 0;
	if (mSlider->mVisible)
	{
		mScrollPosition = ClampFloat(mScrollPosition += mScrollAmount * (mBaseScrollSpeed + abs(mScrollAmount) * mScrollAccel), 0, mMaxScrollPosition);
		mScrollAmount *= (1.0f - mScrollAccel);
		mSlider->SetValue(max(0.0, min(mMaxScrollPosition, mScrollPosition)) / mMaxScrollPosition);
	}
	else
	{
		mScrollPosition = 0;
		mScrollAmount = 0;
	}

	mLastMouseX = mApp->mWidgetManager->mLastMouseX;
	mLastMouseY = mApp->mWidgetManager->mLastMouseY;

	mSeedChooserAge++;
	mToolTip->Update();

	for (SeedType aSeedType = SEED_PEASHOOTER; aSeedType < NUM_SEEDS_IN_CHOOSER; aSeedType = (SeedType)(aSeedType + 1))
	{
		if (mApp->SeedTypeAvailable(aSeedType))
		{
			ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
			if (aChosenSeed.mSeedState == SEED_FLYING_TO_BANK)
			{
				int aTimeStart = aChosenSeed.mTimeStartMotion;
				int aTimeEnd = aChosenSeed.mTimeEndMotion;
				aChosenSeed.mX = TodAnimateCurve(aTimeStart, aTimeEnd, mSeedChooserAge, aChosenSeed.mStartX, aChosenSeed.mEndX, CURVE_EASE_IN_OUT);
				aChosenSeed.mY = TodAnimateCurve(aTimeStart, aTimeEnd, mSeedChooserAge, aChosenSeed.mStartY, aChosenSeed.mEndY, CURVE_EASE_IN_OUT);
				if (mSeedChooserAge >= aChosenSeed.mTimeEndMotion)
				{
					LandFlyingSeed(aChosenSeed);
				}
			}
		}
	}

	ShowToolTip();
	mStartButton->Update();
	mFavButton->Update();
	mSkinButton->Update();
	mStatsButton->Update();
	mRandomButton->Update();
	mViewLawnButton->Update();
	mAlmanacButton->Update();
	mStoreButton->Update();
	mMenuButton->Update();
	if (mPlantPreview && !mApp->mBoard->mPaused) mPlantPreview->Update();
	UpdateViewLawn();
	UpdateCursor();
	if(!mApp->GetDialog(DIALOG_ALMANAC))mApp->mPoolEffect->PoolEffectUpdate();
	MarkDirty();
}

void SeedChooserScreen::MouseWheel(int theDelta)
{
	if (mChooseState != CHOOSE_NORMAL) return;

	mScrollAmount -= mBaseScrollSpeed * theDelta;
	mScrollAmount -= mScrollAmount * mScrollAccel;
}

bool SeedChooserScreen::DisplayRepickWarningDialog(const SexyChar* theMessage)
{
	return mApp->LawnMessageBox(
		Dialogs::DIALOG_CHOOSER_WARNING,
		_S("[DIALOG_WARNING]"),
		theMessage,
		_S("[DIALOG_BUTTON_YES]"),
		_S("[REPICK_BUTTON]"),
		Dialog::BUTTONS_YES_NO
	) == Dialog::ID_YES;
}

bool SeedChooserScreen::FlyersAreComming()
{
	for (int aWave = 0; aWave < mBoard->mNumWaves; aWave++)
	{
		for (int anIndex = 0; anIndex < MAX_ZOMBIES_IN_WAVE; anIndex++)
		{
			ZombieType aZombieType = mBoard->mZombiesInWave[aWave][anIndex];
			if (aZombieType == ZOMBIE_INVALID)
				break;

			if (aZombieType == ZOMBIE_BALLOON)
				return true;
		}
	}
	return false;
}

bool SeedChooserScreen::FlyProtectionCurrentlyPlanted()
{
	Plant* aPlant = nullptr;
	while (mBoard->IteratePlants(aPlant))
	{
		if (aPlant->mSeedType == SEED_CATTAIL || aPlant->mSeedType == SEED_CACTUS)
		{
			return true;
		}
	}
	return false;
}

bool SeedChooserScreen::CheckSeedUpgrade(SeedType theSeedTypeTo, SeedType theSeedTypeFrom)
{
	if (mApp->IsSurvivalMode() || !PickedPlantType(theSeedTypeTo) || PickedPlantType(theSeedTypeFrom))
		return true;

	SexyString aWarning = TodStringTranslate(_S("[SEED_CHOOSER_UPGRADE_WARNING]"));
	aWarning = TodReplaceString(aWarning, _S("{UPGRADE_TO}"), Plant::GetNameString(theSeedTypeTo));
	aWarning = TodReplaceString(aWarning, _S("{UPGRADE_FROM}"), Plant::GetNameString(theSeedTypeFrom));
	return DisplayRepickWarningDialog(aWarning.c_str());
}

void SeedChooserScreen::OnStartButton()
{
	if (mApp->mGameMode == GAMEMODE_CHALLENGE_SEEING_STARS && !PickedPlantType(SEED_STARFRUIT))
	{
		if (!DisplayRepickWarningDialog(_S("[SEED_CHOOSER_SEEING_STARS_WARNING]")))
		{
			return;
		}
	}

	if (mApp->IsFirstTimeAdventureMode() && mBoard->mLevel == 11 && !PickedPlantType(SEED_PUFFSHROOM))
	{
		if (!DisplayRepickWarningDialog(_S("[SEED_CHOOSER_PUFFSHROOM_WARNING]")))
		{
			return;
		}
	}
	if (!PickedPlantType(SEED_SUNFLOWER) && !PickedPlantType(SEED_TWINSUNFLOWER) && !PickedPlantType(SEED_SUNSHROOM) &&
		!mBoard->mCutScene->IsSurvivalRepick() && mApp->mGameMode != GAMEMODE_CHALLENGE_LAST_STAND)
	{
		if (mApp->IsFirstTimeAdventureMode() && mBoard->mLevel == 11)
		{
			if (!DisplayRepickWarningDialog(_S("[SEED_CHOOSER_NIGHT_SUN_WARNING]")))
			{
				return;
			}
		}
		else if (!DisplayRepickWarningDialog(_S("[SEED_CHOOSER_SUN_WARNING]")))
		{
			return;
		}
	}
	if (mBoard->StageHasPool() && !PickedPlantType(SEED_LILYPAD) && !PickedPlantType(SEED_SEASHROOM) && !PickedPlantType(SEED_TANGLEKELP) && !mBoard->mCutScene->IsSurvivalRepick())
	{
		if (mApp->IsFirstTimeAdventureMode() && mBoard->mLevel == 21)
		{
			if (!DisplayRepickWarningDialog(_S("[SEED_CHOOSER_LILY_WARNING]")))
			{
				return;
			}
		}
		else if (!DisplayRepickWarningDialog(_S("[SEED_CHOOSER_POOL_WARNING]")))
		{
			return;
		}
	}
	if (mBoard->StageHasRoof() && !PickedPlantType(SEED_FLOWERPOT) && mApp->SeedTypeAvailable(SEED_FLOWERPOT))
	{
		if (!DisplayRepickWarningDialog(_S("[SEED_CHOOSER_ROOF_WARNING]")))
		{
			return;
		}
	}

	if (mApp->mGameMode == GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT && !PickedPlantType(SEED_WALLNUT))
	{
		if (!DisplayRepickWarningDialog(_S("[SEED_CHOOSER_ART_WALLNUT_WARNING]")))
		{
			return;
		}
	}
	if (mApp->mGameMode == GAMEMODE_CHALLENGE_ART_CHALLENGE_SUNFLOWER &&
		(!PickedPlantType(SEED_STARFRUIT) || !PickedPlantType(SEED_UMBRELLA) || !PickedPlantType(SEED_WALLNUT)))
	{
		if (!DisplayRepickWarningDialog(_S("[SEED_CHOOSER_ART_2_WARNING]")))
		{
			return;
		}
	}

	if (FlyersAreComming() && !FlyProtectionCurrentlyPlanted() && !PickedPlantType(SEED_CATTAIL) && !PickedPlantType(SEED_CACTUS) && !PickedPlantType(SEED_BLOVER))
	{
		if (!DisplayRepickWarningDialog(_S("[SEED_CHOOSER_FLYER_WARNING]")))
		{
			return;
		}
	}

	if (!CheckSeedUpgrade(SEED_GATLINGPEA, SEED_REPEATER) ||
		!CheckSeedUpgrade(SEED_WINTERMELON, SEED_MELONPULT) ||
		!CheckSeedUpgrade(SEED_TWINSUNFLOWER, SEED_SUNFLOWER) ||
		!CheckSeedUpgrade(SEED_SPIKEROCK, SEED_SPIKEWEED) ||
		!CheckSeedUpgrade(SEED_COBCANNON, SEED_KERNELPULT) ||
		!CheckSeedUpgrade(SEED_GOLD_MAGNET, SEED_MAGNETSHROOM) ||
		!CheckSeedUpgrade(SEED_GLOOMSHROOM, SEED_FUMESHROOM) ||
		!CheckSeedUpgrade(SEED_CATTAIL, SEED_LILYPAD))
		return;

	CloseSeedChooser();
}

void SeedChooserScreen::PickRandomSeeds()
{
	for (int anIndex = mSeedsInBank; anIndex < mBoard->mSeedBank->mNumPackets; anIndex++)
	{
		SeedType aSeedType;
		do aSeedType = (SeedType)Rand(mApp->GetSeedsAvailable());
		while (!mApp->SeedTypeAvailable(aSeedType) || aSeedType == SEED_IMITATER || mChosenSeeds[aSeedType].mSeedState != SEED_IN_CHOOSER);
		ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
		aChosenSeed.mTimeStartMotion = 0;
		aChosenSeed.mTimeEndMotion = 0;
		aChosenSeed.mStartX = aChosenSeed.mX;
		aChosenSeed.mStartY = aChosenSeed.mY;
		GetSeedPositionInBank(anIndex, aChosenSeed.mEndX, aChosenSeed.mEndY);
		aChosenSeed.mSeedState = SEED_IN_BANK;
		aChosenSeed.mSeedIndexInBank = anIndex;
		mSeedsInBank++;
	}
	for (SeedType aSeedFlying = SEED_PEASHOOTER; aSeedFlying < NUM_SEEDS_IN_CHOOSER; aSeedFlying = (SeedType)(aSeedFlying + 1))
		LandFlyingSeed(mChosenSeeds[aSeedFlying]);
	CloseSeedChooser();
}

void SeedChooserScreen::ButtonDepress(int theId)
{
	if (mSeedsInFlight > 0 || mChooseState == CHOOSE_VIEW_LAWN || !mMouseVisible)
		return;

	if (theId == SeedChooserScreen::SeedChooserScreen_ViewLawn)
	{
		mChooseState = CHOOSE_VIEW_LAWN;
		mMenuButton->mDisabled = true;
		mViewLawnTime = 0;
	}
	else if (theId == SeedChooserScreen::SeedChooserScreen_Almanac)
	{
		mApp->DoAlmanacDialog()->WaitForResult(true);
		mApp->mWidgetManager->SetFocus(this);
	}
	else if (theId == SeedChooserScreen::SeedChooserScreen_Store)
	{
		StoreScreen* aStore = mApp->ShowStoreScreen();
		aStore->mBackButton->mLabel = _S("[STORE_BACK_TO_GAME]");
		aStore->WaitForResult();
		if (aStore->mGoToTreeNow)
		{
			mApp->KillBoard();
			mApp->PreNewGame(GAMEMODE_TREE_OF_WISDOM, false);
		}
		else
		{
			mApp->mMusic->MakeSureMusicIsPlaying(MUSIC_TUNE_CHOOSE_YOUR_SEEDS);
			mApp->mWidgetManager->SetFocus(this);
		}
	}
	else if (theId == SeedChooserScreen::SeedChooserScreen_Menu)
	{
		mMenuButton->mIsOver = false;
		mMenuButton->mIsDown = false;
		UpdateCursor();
		mApp->DoNewOptions(false);
	}
	else if (theId == SeedChooserScreen::SeedChooserScreen_Fav)
	{
		mFavButton->mIsOver = false;
		mFavButton->mIsDown = false;
	}
	else if (theId == SeedChooserScreen::SeedChooserScreen_Skin)
	{
		mSkinButton->mIsOver = false;
		mSkinButton->mIsDown = false;
	}
	else if (theId == SeedChooserScreen::SeedChooserScreen_Stats)
	{
		mStatsButton->mIsOver = false;
		mStatsButton->mIsDown = false;
	}
	else if (mApp->GetSeedsAvailable() >= mBoard->mSeedBank->mNumPackets)
	{
		if (theId == SeedChooserScreen::SeedChooserScreen_Start)
			OnStartButton();
		else if (theId == SeedChooserScreen::SeedChooserScreen_Random)
			PickRandomSeeds();
	}
}

SeedType SeedChooserScreen::SeedHitTest(int x, int y)
{
	if (mOrderedSeedsDirty) RebuildOrderedSeeds();
	if (mMouseVisible)
	{
		for (int i = 0;i < mOrderedSeeds.size();i++)
		{
			SeedType aSeedType = mOrderedSeeds[i];
			ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
			if (!mApp->SeedTypeAvailable(aSeedType) || aChosenSeed.mSeedState == SEED_PACKET_HIDDEN) continue;
			if (aChosenSeed.mSeedState == SEED_IN_CHOOSER)
			{
				int aPosX, aPosY;
				GetSeedPositionInChooser(i, aPosX, aPosY);
				Rect aChosenSeedRect = Rect(aPosX, aPosY, SEED_PACKET_WIDTH, SEED_PACKET_HEIGHT);
				if ((aChosenSeed.mSeedType != SEED_IMITATER ? cSeedClipRect.Contains(x, y) : true) && aChosenSeedRect.Contains(x, y))
				{
					return aSeedType;
				}
			}
			else
			{
				if (Rect(aChosenSeed.mX, aChosenSeed.mY, SEED_PACKET_WIDTH, SEED_PACKET_HEIGHT).Contains(x, y)) return aSeedType;
			}
		}
	}
	return SEED_NONE;
}

SeedType SeedChooserScreen::FindSeedInBank(int theIndexInBank)
{
	for (SeedType aSeedType = SEED_PEASHOOTER; aSeedType < NUM_SEEDS_IN_CHOOSER; aSeedType = (SeedType)(aSeedType + 1))
	{
		if (mApp->SeedTypeAvailable(aSeedType))
		{
			ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
			if (aChosenSeed.mSeedState == SEED_IN_BANK && aChosenSeed.mSeedIndexInBank == theIndexInBank) return aSeedType;
		}
	}
	return SEED_NONE;
}

void SeedChooserScreen::EnableStartButton(bool theEnabled)
{
	mStartButton->SetDisabled(!theEnabled);
	if (theEnabled) mStartButton->mColors[GameButton::COLOR_LABEL] = Color::White;
	else mStartButton->mColors[GameButton::COLOR_LABEL] = Color(64, 64, 64);
}

void SeedChooserScreen::ClickedSeedInBank(ChosenSeed& theChosenSeed)
{
	mPreviousType = FindSeedInBank(mSeedsInBank - (theChosenSeed.mSeedIndexInBank == mSeedsInBank - 1 ? 2 : 1));
	for (int anIndex = theChosenSeed.mSeedIndexInBank + 1; anIndex < mBoard->mSeedBank->mNumPackets; anIndex++)
	{
		SeedType aSeedType = FindSeedInBank(anIndex);
		if (aSeedType != SEED_NONE)
		{
			ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
			aChosenSeed.mTimeStartMotion = mSeedChooserAge;
			aChosenSeed.mTimeEndMotion = mSeedChooserAge + 15;
			aChosenSeed.mStartX = aChosenSeed.mX;
			aChosenSeed.mStartY = aChosenSeed.mY;
			GetSeedPositionInBank(anIndex - 1, aChosenSeed.mEndX, aChosenSeed.mEndY);
			aChosenSeed.mSeedState = SEED_FLYING_TO_BANK;
			aChosenSeed.mSeedIndexInBank = anIndex - 1;
			mSeedsInFlight++;
		}
	}
	//theChosenSeed.mTimeStartMotion = mSeedChooserAge;
	//theChosenSeed.mTimeEndMotion = mSeedChooserAge + 25;
	//theChosenSeed.mStartX = theChosenSeed.mX;
	//theChosenSeed.mStartY = theChosenSeed.mY;
	GetSeedPositionInChooser(theChosenSeed.mSeedType, theChosenSeed.mX, theChosenSeed.mY);
	theChosenSeed.mY += theChosenSeed.mSeedType == SEED_IMITATER ? 0 : mScrollPosition;
	theChosenSeed.mSeedState = SEED_IN_CHOOSER;
	theChosenSeed.mImitaterType = SEED_NONE;
	theChosenSeed.mSeedIndexInBank = 0;
	mSeedsInBank--;
	//mSeedsInFlight++;
	RemoveToolTip();
	EnableStartButton(false);
	mPreviewSeed = theChosenSeed.mSeedType;
	mFavButton->mButtonImage = !mApp->mPlayerInfo->mFavoriteSeeds[mPreviewSeed] ? Sexy::IMAGE_SEEDCHOOSER_BUTTON_FAV : Sexy::IMAGE_SEEDCHOOSER_BUTTON_FAV_ACTIVE;
	if (mPreviewSeed == SEED_IMITATER) mFavButton->SetDisabled(true);
	else mFavButton->SetDisabled(false);
	mSkinButton->SetDisabled(!PlantHasSkin(mPreviewSeed, 1));
	SetupPlantPreview();
	mApp->PlaySample(Sexy::SOUND_TAP);
}

void SeedChooserScreen::ClickedSeedInChooser(ChosenSeed& theChosenSeed)
{
	mPreviewSeed = theChosenSeed.mSeedType;
	mFavButton->mButtonImage = !mApp->mPlayerInfo->mFavoriteSeeds[mPreviewSeed] ? Sexy::IMAGE_SEEDCHOOSER_BUTTON_FAV : Sexy::IMAGE_SEEDCHOOSER_BUTTON_FAV_ACTIVE;
	if (mPreviewSeed == SEED_IMITATER) mFavButton->SetDisabled(true);
	else mFavButton->SetDisabled(false);
	mSkinButton->SetDisabled(!PlantHasSkin(mPreviewSeed, 1));
	SetupPlantPreview();
	mApp->PlaySample(Sexy::SOUND_TAP);

	if (mSeedsInBank == mBoard->mSeedBank->mNumPackets)
		return;

	if (IsImitaterUnselectable(theChosenSeed.mSeedType))
		return;

	if (theChosenSeed.mSeedType == SEED_IMITATER)
		theChosenSeed.mImitaterType = FindSeedInBank(mSeedsInBank - 1);
	else
		mPreviousType = theChosenSeed.mSeedType;

	theChosenSeed.mTimeStartMotion = mSeedChooserAge;
	theChosenSeed.mTimeEndMotion = mSeedChooserAge + 25;
	int visualIndex = GetSeedVisualIndex(theChosenSeed.mSeedType);
	if (visualIndex < 0)	return;
	GetSeedPositionInChooser(visualIndex, theChosenSeed.mStartX, theChosenSeed.mStartY);
	GetSeedPositionInBank(mSeedsInBank, theChosenSeed.mEndX, theChosenSeed.mEndY);
	theChosenSeed.mSeedState = SEED_FLYING_TO_BANK;
	theChosenSeed.mSeedIndexInBank = mSeedsInBank;
	mSeedsInFlight++;
	mSeedsInBank++;

	RemoveToolTip();
	if (mSeedsInBank == mBoard->mSeedBank->mNumPackets)
		EnableStartButton(true);
}

void SeedChooserScreen::ShowToolTip()
{
	if (!mApp->mWidgetManager->mMouseIn || !mApp->mActive || mApp->GetDialogCount() > 0 || mChooseState == CHOOSE_VIEW_LAWN)
	{
		RemoveToolTip();
		return;
	}

	Zombie* aZombie = ZombieHitTest(mLastMouseX, mLastMouseY);
	if (aZombie == nullptr || aZombie->mFromWave != Zombie::ZOMBIE_WAVE_CUTSCENE)
	{
		RemoveToolTip();
	}
	else if (!IsOverImitater(mLastMouseX, mLastMouseY))
	{

		SexyString aZombieName = StrFormat(_S("[%s]"), GetZombieDefinition(aZombie->mZombieType).mZombieName);
		mToolTip->SetTitle(aZombieName);
		if (mApp->CanShowAlmanac())
		{
			mToolTip->SetLabel(_S("[CLICK_TO_VIEW]"));
		}
		else
		{
			mToolTip->SetLabel(_S(""));
		}
		mToolTip->SetWarningText(_S(""));

		Rect aRect = aZombie->GetZombieRect();
		mToolTip->mX = aRect.mWidth / 2 + aRect.mX + 5 + mBoard->mX;
		mToolTip->mY = aRect.mHeight + aRect.mY - 10 - mBoard->mY;
		if (aZombie->mZombieType == ZombieType::ZOMBIE_BUNGEE)
			mToolTip->mY = aZombie->mY;
		mToolTip->mCenter = true;
		mToolTip->mVisible = true;
		if (mAlmanacButton->mBtnNoDraw && mStoreButton->mBtnNoDraw)
			mToolTip->mMaxBottom = BOARD_HEIGHT;
		else
			mToolTip->mMaxBottom = BOARD_HEIGHT - 30;
		return;
	}

	if (mSeedsInFlight <= 0)
	{
		SeedType aSeedType = SeedHitTest(mLastMouseX, mLastMouseY);
		if (aSeedType == SEED_NONE)
		{
			RemoveToolTip();
		}
		else if (aSeedType != mToolTipSeed)
		{
			RemoveToolTip();
			ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
			uint aRecFlags = SeedNotRecommendedToPick(aSeedType);
			if (SeedNotAllowedToPick(aSeedType))
			{
				mToolTip->SetWarningText(_S("[NOT_ALLOWED_ON_THIS_LEVEL]"));
			}
			else if (SeedNotAllowedDuringTrial(aSeedType))
			{
				mToolTip->SetWarningText(_S("[FULL_VERSION_ONLY]"));
			}
			else if (aChosenSeed.mSeedState == SEED_IN_BANK && aChosenSeed.mCrazyDavePicked)
			{
				mToolTip->SetWarningText(_S("[CRAZY_DAVE_WANTS]"));
			}
			else if (aRecFlags != 0U)
			{
				if (TestBit(aRecFlags, NOT_RECOMMENDED_NOCTURNAL))
				{
					mToolTip->SetWarningText(_S("[NOCTURNAL_WARNING]"));
				}
				else
				{
					mToolTip->SetWarningText(_S("[NOT_RECOMMENDED_FOR_LEVEL]"));
				}
			}
			else
			{
				mToolTip->SetWarningText(_S(""));
			}

			if (aSeedType == SEED_IMITATER)
			{
				mToolTip->SetTitle(Plant::GetNameString(aSeedType, aChosenSeed.mImitaterType));
				mToolTip->SetLabel(_S(""));
				//mToolTip->SetLabel(Plant::GetToolTip(aChosenSeed.mImitaterType == SEED_NONE ? SEED_IMITATER : aChosenSeed.mImitaterType));
			}
			else
			{
				mToolTip->SetTitle(Plant::GetNameString(aSeedType, SEED_NONE));
				mToolTip->SetLabel(_S(""));
				//mToolTip->SetLabel(Plant::GetToolTip(aSeedType));
			}

			int aSeedX, aSeedY;
			int visualIndex = GetSeedVisualIndex(aSeedType);
			if (visualIndex < 0) return;
			if (aChosenSeed.mSeedState == SEED_IN_BANK)
			{
				GetSeedPositionInBank(aChosenSeed.mSeedIndexInBank, aSeedX, aSeedY);
			}
			else
			{
				GetSeedPositionInChooser(visualIndex, aSeedX, aSeedY);
			}

			mToolTip->mX = ClampInt((SEED_PACKET_WIDTH - mToolTip->mWidth) / 2 + aSeedX, 0, BOARD_WIDTH - mToolTip->mWidth);
			mToolTip->mY = aSeedY + (aSeedType == SEED_IMITATER && aChosenSeed.mSeedState == SEED_IN_CHOOSER ? -mToolTip->mHeight : SEED_PACKET_HEIGHT);
			mToolTip->mVisible = true;
			mToolTipSeed = aSeedType;
		}
		else if (aSeedType == mToolTipSeed)
		{
			ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
			if (aChosenSeed.mSeedState != SEED_IN_CHOOSER) return;

			// Update tooltip pos since seeds in the chooser might be moving
			int aSeedX, aSeedY;
			GetSeedPositionInChooser(aSeedType, aSeedX, aSeedY);

			mToolTip->mX = ClampInt((SEED_PACKET_WIDTH - mToolTip->mWidth) / 2 + aSeedX, 0, BOARD_WIDTH - mToolTip->mWidth);
			mToolTip->mY = aSeedY + 70;
		}
	}
}


void SeedChooserScreen::RemoveToolTip()
{
	mToolTip->mVisible = false;
	mToolTip->mMaxBottom = BOARD_HEIGHT;
	mToolTip->mCenter = false;
	mToolTipSeed = SEED_NONE;
}

void SeedChooserScreen::CancelLawnView()
{
	if (mChooseState == CHOOSE_VIEW_LAWN && mViewLawnTime > 100 && mViewLawnTime <= 250) mViewLawnTime = 251;
}

bool SeedChooserScreen::IsOverImitater(int x, int y)
{
	return mApp->SeedTypeAvailable(SEED_IMITATER) && Rect(IMITATER_POS_X, IMITATER_POS_Y + SEED_CHOOSER_EXTRA_HEIGHT, IMAGE_SEEDCHOOSER_IMITATERADDON->mWidth, IMAGE_SEEDCHOOSER_IMITATERADDON->mHeight).Contains(x, y);
}

void SeedChooserScreen::ResizeSlider()
{
	mSlider->Resize(472, 92, 40, IMAGE_SEEDCHOOSER_BACKGROUND->mHeight - (mApp->SeedTypeAvailable(SEED_IMITATER) ? IMAGE_SEEDCHOOSER_IMITATERADDON->mHeight + 4 : 0) - 11);
}

void SeedChooserScreen::MouseUp(int x, int y, int theClickCount)
{
	if (theClickCount == 1)
	{
		if (mMenuButton->IsMouseOver()) ButtonDepress(SeedChooserScreen::SeedChooserScreen_Menu);
		else if (mStartButton->IsMouseOver()) ButtonDepress(SeedChooserScreen::SeedChooserScreen_Start);
		else if (mFavButton->IsMouseOver()) { 
			mApp->mPlayerInfo->ToggleFavoriteSeed(mPreviewSeed); 
			mFavButton->mButtonImage = !mApp->mPlayerInfo->mFavoriteSeeds[mPreviewSeed] ? Sexy::IMAGE_SEEDCHOOSER_BUTTON_FAV : Sexy::IMAGE_SEEDCHOOSER_BUTTON_FAV_ACTIVE;
			mOrderedSeedsDirty = true;
			ButtonDepress(SeedChooserScreen::SeedChooserScreen_Fav); }
		else if (mSkinButton->IsMouseOver()) {
			mApp->mPlayerInfo->SwitchSeedCostume(mPreviewSeed);
			mApp->InvalidatePlantPreview(mPreviewSeed);
			mPreviewSkin = mApp->mPlayerInfo->mSeedsSkin[(int)mPreviewSeed];
			SetupPlantPreview();
			ButtonDepress(SeedChooserScreen::SeedChooserScreen_Skin);
		}
		else if (mStatsButton->IsMouseOver()) { 
			mApp->mPlayerInfo->ToggleStatsMode(); 
			mStatsButton->mButtonImage = !mApp->mPlayerInfo->mShowStats ? Sexy::IMAGE_SEEDCHOOSER_BUTTON_STAT : Sexy::IMAGE_SEEDCHOOSER_BUTTON_STAT_ACTIVE; 
			ButtonDepress(SeedChooserScreen::SeedChooserScreen_Stats); }
		else if (mAlmanacButton->IsMouseOver()) ButtonDepress(SeedChooserScreen::SeedChooserScreen_Almanac);
		else if (mStoreButton->IsMouseOver()) ButtonDepress(SeedChooserScreen::SeedChooserScreen_Store);
	}
}

bool SeedChooserScreen::IsImitaterUnselectable(SeedType seedType)
{
	return seedType == SEED_IMITATER && (mSeedsInBank == 0 || mSeedsInBank == mBoard->mSeedBank->mNumPackets || Plant::IsUpgrade(mPreviousType) || SeedNotAllowedToPick(mPreviousType));
}

void SeedChooserScreen::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);

	if (mSeedsInFlight > 0)
	{
		for (int i = 0; i < NUM_SEEDS_IN_CHOOSER; i++)
		{
			LandFlyingSeed(mChosenSeeds[i]);
		}
	}

	if (mChooseState == CHOOSE_VIEW_LAWN)
	{
		CancelLawnView();
	}
	else if (mRandomButton->IsMouseOver())
	{
		mApp->PlaySample(Sexy::SOUND_TAP);
		ButtonDepress(SeedChooserScreen::SeedChooserScreen_Random);
	}
	else if (mFavButton->IsMouseOver())
	{
		mApp->PlaySample(Sexy::SOUND_TAP);
		ButtonDepress(SeedChooserScreen::SeedChooserScreen_Fav);
	}
	else if (mSkinButton->IsMouseOver())
	{
		mApp->PlaySample(Sexy::SOUND_TAP);
		ButtonDepress(SeedChooserScreen::SeedChooserScreen_Skin);
	}
	else if (mStatsButton->IsMouseOver())
	{
		mApp->PlaySample(Sexy::SOUND_TAP);
		ButtonDepress(SeedChooserScreen::SeedChooserScreen_Stats);
	}
	else if (mViewLawnButton->IsMouseOver())
	{
		mApp->PlaySample(Sexy::SOUND_TAP);
		ButtonDepress(SeedChooserScreen::SeedChooserScreen_ViewLawn);
	}
	else if (mMenuButton->IsMouseOver())
	{
		mApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
	}
	else if (mStartButton->IsMouseOver() || mAlmanacButton->IsMouseOver() || mFavButton->IsMouseOver() || mSkinButton->IsMouseOver() || mStatsButton->IsMouseOver())
	{
		mApp->PlaySample(Sexy::SOUND_TAP);
	}
	else if (mStoreButton->IsMouseOver()) //WIDETWEAK: fixed scrolling CYS hitbox bug after visiting shop (i can't actually test this out so if there's an issue with this please report to me)
	{
		mApp->PlaySample(Sexy::SOUND_TAP);
		mScrollAmount = 0;
		mScrollPosition = 0;
	}
	else
	{
		if (!IsOverImitater(x, y) && !mAlmanacButton->IsMouseOver() && !mStoreButton->IsMouseOver() && mApp->CanShowAlmanac())
		{
			Zombie* aZombie = ZombieHitTest(x, y);
			if (aZombie && aZombie->mFromWave == Zombie::ZOMBIE_WAVE_CUTSCENE && aZombie->mZombieType != ZOMBIE_REDEYE_GARGANTUAR)
			{
				mApp->PlaySample(Sexy::SOUND_TAP);
				mApp->DoAlmanacDialog(SEED_NONE, aZombie->mZombieType)->WaitForResult(true);
				mApp->mWidgetManager->SetFocus(this);
				return;
			}
		}

		SeedType aSeedType = SeedHitTest(x, y);
		if (aSeedType != SEED_NONE && !SeedNotAllowedToPick(aSeedType))
		{
			if (SeedNotAllowedDuringTrial(aSeedType))
			{
				mApp->PlaySample(Sexy::SOUND_TAP);
				if (mApp->LawnMessageBox(
					DIALOG_MESSAGE,
					_S("[GET_FULL_VERSION_TITLE]"),
					_S("[GET_FULL_VERSION_BODY]"),
					_S("[GET_FULL_VERSION_YES_BUTTON]"),
					_S("[GET_FULL_VERSION_NO_BUTTON]"),
					Dialog::BUTTONS_YES_NO
				) == Dialog::ID_YES)
				{
					if (mApp->mDRM)
					{
						mApp->mDRM->BuyGame();
					}
					mApp->DoBackToMain();
				}
			}
			else
			{
				ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
				if (aChosenSeed.mSeedState == SEED_IN_BANK)
				{
					if (aChosenSeed.mCrazyDavePicked)
					{
						mApp->PlaySample(Sexy::SOUND_BUZZER);
						mToolTip->FlashWarning();
					}
					else ClickedSeedInBank(aChosenSeed);
				}
				else if (aChosenSeed.mSeedState == SEED_IN_CHOOSER)
					ClickedSeedInChooser(aChosenSeed);
			}
		}
	}
}

bool SeedChooserScreen::PickedPlantType(SeedType theSeedType)
{
	for (SeedType aSeedType = SEED_PEASHOOTER; aSeedType < NUM_SEEDS_IN_CHOOSER; aSeedType = (SeedType)(aSeedType + 1))
	{
		ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
		if (aChosenSeed.mSeedState == SEED_IN_BANK || aChosenSeed.mSeedState == SEED_FLYING_TO_BANK)
		{
			if (aChosenSeed.mSeedType == theSeedType || (aChosenSeed.mSeedType == SEED_IMITATER && aChosenSeed.mImitaterType == theSeedType))
			{
				return true;
			}
		}
	}
	return false;
}

void SeedChooserScreen::CloseSeedChooser()
{
	DBG_ASSERT(mBoard->mSeedBank->mNumPackets == mBoard->GetNumSeedsInBank());
	for (int anIndex = 0; anIndex < mBoard->mSeedBank->mNumPackets; anIndex++)
	{
		SeedType aSeedType = FindSeedInBank(anIndex);
		ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
		SeedPacket& aSeedPacket = mBoard->mSeedBank->mSeedPackets[anIndex];
		aSeedPacket.SetPacketType(aSeedType, aChosenSeed.mImitaterType);
		if (aChosenSeed.mRefreshing)
		{
			aSeedPacket.mRefreshCounter = aChosenSeed.mRefreshCounter;
			aSeedPacket.mRefreshTime = Plant::GetRefreshTime(aSeedPacket.mPacketType, aSeedPacket.mImitaterType);
			aSeedPacket.mRefreshing = true;
			aSeedPacket.mActive = false;
		}
	}
	mBoard->mCutScene->EndSeedChooser();
}

void SeedChooserScreen::KeyDown(KeyCode theKey)
{
	mBoard->DoTypingCheck(theKey);
}

void SeedChooserScreen::KeyChar(SexyChar theChar)
{
	if (mChooseState == CHOOSE_VIEW_LAWN && (theChar == ' ' || theChar == '\r' || theChar == '\u001B'))
		CancelLawnView();
	else if (mApp->mTodCheatKeys && theChar == '\u001B')
		PickRandomSeeds();
	else mBoard->KeyChar(theChar);
}

void SeedChooserScreen::UpdateAfterPurchase()
{
	for (SeedType aSeedType = SEED_PEASHOOTER; aSeedType < NUM_SEEDS_IN_CHOOSER; aSeedType = (SeedType)(aSeedType + 1))
	{
		ChosenSeed& aChosenSeed = mChosenSeeds[aSeedType];
		if (aChosenSeed.mSeedState == SEED_IN_BANK)
			GetSeedPositionInBank(aChosenSeed.mSeedIndexInBank, aChosenSeed.mX, aChosenSeed.mY);
		else if (aChosenSeed.mSeedState == SEED_IN_CHOOSER)
			GetSeedPositionInChooser(aSeedType, aChosenSeed.mX, aChosenSeed.mY);
		else continue;
		aChosenSeed.mStartX = aChosenSeed.mX;
		aChosenSeed.mStartY = aChosenSeed.mY;
		aChosenSeed.mEndX = aChosenSeed.mX;
		aChosenSeed.mEndY = aChosenSeed.mY;
	}
	EnableStartButton(mSeedsInBank == mBoard->mSeedBank->mNumPackets);
	ResizeSlider();
}

Zombie* SeedChooserScreen::ZombieHitTest(int x, int y)
{
	Zombie* aRecord = nullptr;
	if (mMouseVisible && !Rect(mBoard->mSeedBank->mX - mX, mBoard->mSeedBank->mY - mY, mBoard->mSeedBank->mWidth, mBoard->mSeedBank->mHeight).Contains(x, y))
	{
		Zombie* aZombie = nullptr;
		while (mBoard->IterateZombies(aZombie))
		{
			if (aZombie->mDead || aZombie->IsDeadOrDying() || aZombie->mZombieType >= NUM_ZOMBIES_IN_ALMANAC)
				continue;

			if (aZombie->GetZombieRect().Contains(x - mBoard->mX, y - mBoard->mY))
			{
				if (aRecord == nullptr || aZombie->mY > aRecord->mY)
				{
					aRecord = aZombie;
				}
			}
		}
	}
	return aRecord;
}


void SeedChooserScreen::SliderVal(int theId, double theVal)
{
	switch (theId)
	{
	case 0:
		mScrollPosition = theVal * mMaxScrollPosition;
		break;
	}
}

void SeedChooserScreen::SetupPlantPreview() {

	if (mPlantPreview != nullptr &&
		mPlantPreview->mSeedType == mPreviewSeed &&
		(!PlantHasSkin(mPreviewSeed, 1) ||
			mPlantPreview->mSkinType == mPreviewSkin))
		return;

	float aPosX = 94;
	float aPosY = 158;

	switch (mPreviewSeed)
	{
	case SEED_TALLNUT: aPosY += 16; break;
	case SEED_COBCANNON: aPosX -= 30; break;
	case SEED_FLOWERPOT: aPosY -= 20; break;
	case SEED_LILYPAD:
	case SEED_TANGLEKELP: 
	case SEED_SEASHROOM: aPosY -= 5; break;
	case SEED_INSTANT_COFFEE: aPosY += 5; break;
	case SEED_GRAVEBUSTER: aPosY += 50; aPosX += 5; break;
	case SEED_SCAREDYSHROOM: aPosY += 8; aPosX += 6; break;
	case SEED_MELONPULT:
	case SEED_WINTERMELON: aPosY += 8; break;
	}

	mPlantPreview = new Plant();
	mPlantPreview->mBoard = nullptr;
	mPlantPreview->mIsOnBoard = false;
	mPlantPreview->PlantInitialize(0, 0, mPreviewSeed, SEED_NONE);
	mPlantPreview->mX = aPosX;
	mPlantPreview->mY = aPosY;
}

void SeedChooserScreen::RefreshButtons(){
	mFavButton->mButtonImage = !mApp->mPlayerInfo->mFavoriteSeeds[mPreviewSeed] ? Sexy::IMAGE_SEEDCHOOSER_BUTTON_FAV : Sexy::IMAGE_SEEDCHOOSER_BUTTON_FAV_ACTIVE;
	mStatsButton->mButtonImage = !mApp->mPlayerInfo->mShowStats ? Sexy::IMAGE_SEEDCHOOSER_BUTTON_STAT : Sexy::IMAGE_SEEDCHOOSER_BUTTON_STAT_ACTIVE;
}

void SeedChooserScreen::RebuildOrderedSeeds()
{
	mOrderedSeeds.clear();

	//fav seeds
	for (SeedType aSeedType = SEED_PEASHOOTER;
		aSeedType < NUM_SEEDS_IN_CHOOSER;
		aSeedType = (SeedType)(aSeedType + 1))
	{
		if (mApp->mPlayerInfo->mFavoriteSeeds[aSeedType])
			mOrderedSeeds.push_back(aSeedType);
	}

	//normal seeds
	for (SeedType aSeedType = SEED_PEASHOOTER;
		aSeedType < NUM_SEEDS_IN_CHOOSER;
		aSeedType = (SeedType)(aSeedType + 1))
	{
		if (!mApp->mPlayerInfo->mFavoriteSeeds[aSeedType])
			mOrderedSeeds.push_back(aSeedType);
	}

	mOrderedSeedsDirty = false;
}

int SeedChooserScreen::GetSeedVisualIndex(SeedType type)
{
	for (int i = 0; i < mOrderedSeeds.size(); i++)
		if (mOrderedSeeds[i] == type)
			return i;

	return -1;
}