package com.android.car.dialer.ui.activecall;

import android.os.Bundle;
import android.telecom.Call;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.ViewModelProvider;

import com.android.car.dialer.R;

import dagger.hilt.android.AndroidEntryPoint;

/** Controller bar for the incoming call. */
@AndroidEntryPoint(Fragment.class)
public class RingingCallControllerBarFragment extends Hilt_RingingCallControllerBarFragment {

    private LiveData<Call> mIncomingCall;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View fragmentView = inflater.inflate(R.layout.ringing_call_controller_bar_fragment,
                container, false);

        fragmentView.findViewById(R.id.answer_call_button).setOnClickListener((v) -> answerCall());
        fragmentView.findViewById(R.id.answer_call_text).setOnClickListener((v) -> answerCall());
        fragmentView.findViewById(R.id.end_call_button).setOnClickListener((v) -> declineCall());
        fragmentView.findViewById(R.id.end_call_text).setOnClickListener((v) -> declineCall());

        return fragmentView;
    }

    @Override
    public void onStart() {
        super.onStart();
        InCallViewModel inCallViewModel = new ViewModelProvider(getActivity()).get(
                InCallViewModel.class);
        mIncomingCall = inCallViewModel.getIncomingCall();
    }

    private void answerCall() {
        if (mIncomingCall.getValue() != null) {
            mIncomingCall.getValue().answer(/* videoState= */0);
        }
    }

    private void declineCall() {
        if (mIncomingCall.getValue() != null) {
            mIncomingCall.getValue().reject(/* rejectWithMessage= */false, /* textMessage= */null);
        }
    }
}
