/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include "ROSConDemoSystemComponent.h"
#include "ApplePicker/GatheringRowRequests.h"
#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <ROS2/ROS2Bus.h>
#include <ROS2/Utilities/ROS2Conversions.h>
#include <ILevelSystem.h>
#include <ISystem.h>

namespace ROSConDemo
{
    void ROSConDemoSystemComponent::Reflect(AZ::ReflectContext* context)
    {

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ROSConDemoSystemComponent, AZ::Component>()->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ROSConDemoSystemComponent>("ROSConDemo", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ROSConDemoRequestBus>("ROSConDemoRequestBus")
                ->Event("ReloadLevel", &ROSConDemoRequestBus::Events::ReloadLevel);
        }
    }

    void ROSConDemoSystemComponent::ReloadLevel()
    {
        ISystem* systemInterface = nullptr;
        CrySystemRequestBus::BroadcastResult(systemInterface, &CrySystemRequests::GetCrySystem);
        if(systemInterface && systemInterface->GetILevelSystem())
        {
            ILevelSystem* levelSystem = systemInterface->GetILevelSystem();
            AZStd::string currentLevelName = levelSystem->GetCurrentLevelName();
            levelSystem->UnloadLevel();
            AZ::TickBus::QueueFunction([levelSystem, currentLevelName]() {
                levelSystem->LoadLevel(currentLevelName.c_str());
            });
        }
    }

    void ROSConDemoSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {   
        provided.push_back(AZ_CRC("ROSConDemoService"));
    }

    void ROSConDemoSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ROSConDemoService"));
    }

    void ROSConDemoSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("ROS2Service"));

    }

    void ROSConDemoSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ROSConDemoSystemComponent::ROSConDemoSystemComponent()
    {
        if (ROSConDemoInterface::Get() == nullptr)
        {
            ROSConDemoInterface::Register(this);
        }
    }

    ROSConDemoSystemComponent::~ROSConDemoSystemComponent()
    {
        if (ROSConDemoInterface::Get() == this)
        {
            ROSConDemoInterface::Unregister(this);
        }
    }

    void ROSConDemoSystemComponent::ProcessGetPlanServiceCall(const GetPlanRequestPtr req, GetPlanResponsePtr resp)
    {
        AZ::EBusAggregateResults<AppleKraken::GatheringPoses> results;
        AppleKraken::GatheringRowRequestBus::BroadcastResult(results, &AppleKraken::GatheringRowRequests::GetGatheringPoses);

        // remove all empty rows to avoid checking it later
        results.values.erase(
            std::remove_if(
                results.values.begin(),
                results.values.end(),
                [](const auto& row){ return row.empty(); }),
            results.values.end());

        if (results.values.empty())
        {
            // there are no gathering rows containing at least one pose detected
            return;
        }

        auto startTransform = ROS2::ROS2Conversions::FromROS2Pose(req->start.pose);

        auto nearest_row = *std::min_element(
            results.values.begin(),
            results.values.end(),
            [startTranslation = startTransform.GetTranslation()](const auto& row1,const auto& row2)
            {
                if (row1[0].GetTranslation().GetDistance(startTranslation) < row2[0].GetTranslation().GetDistance(startTranslation))
                {
                   return true;
                }
                return false;
            });

        for( const auto& pose : nearest_row)
        {
            geometry_msgs::msg::PoseStamped stampedPose; // TODO - fill in header
            stampedPose.pose = ROS2::ROS2Conversions::ToROS2Pose(pose);
            resp->plan.poses.push_back(stampedPose);
        }
    }

    void ROSConDemoSystemComponent::Activate()
    {
        ROSConDemoRequestBus::Handler::BusConnect();
        auto ros2Node = ROS2::ROS2Interface::Get()->GetNode();
        m_pathPlanService = ros2Node->create_service<nav_msgs::srv::GetPlan>(
            m_planTopic.c_str(),
            [this](const GetPlanRequestPtr request, GetPlanResponsePtr response)
            {
                this->ProcessGetPlanServiceCall(request, response);
            });
    }

    void ROSConDemoSystemComponent::Deactivate()
    {
        m_pathPlanService.reset();
        ROSConDemoRequestBus::Handler::BusDisconnect();
    }
} // namespace ROSConDemo
