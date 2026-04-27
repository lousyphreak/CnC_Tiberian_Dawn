<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="@TD_ANDROID_PACKAGE@">

    <uses-feature android:glEsVersion="0x00020000" />

    <uses-feature
        android:name="android.hardware.touchscreen"
        android:required="false" />
    <uses-feature
        android:name="android.hardware.bluetooth"
        android:required="false" />
    <uses-feature
        android:name="android.hardware.gamepad"
        android:required="false" />
    <uses-feature
        android:name="android.hardware.usb.host"
        android:required="false" />
    <uses-feature
        android:name="android.hardware.type.pc"
        android:required="false" />

    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
    <uses-permission android:name="android.permission.VIBRATE" />

    <application
        android:allowBackup="true"
        android:hasCode="true"
        android:icon="@mipmap/ic_launcher"
        android:roundIcon="@mipmap/ic_launcher"
        android:label="Tiberian Dawn"
        android:theme="@style/AppTheme"
        android:enableOnBackInvokedCallback="false"
        android:hardwareAccelerated="true">
        <activity
            android:name="@TD_ANDROID_PACKAGE@.@TD_ANDROID_ACTIVITY@"
            android:label="Tiberian Dawn"
            android:alwaysRetainTaskState="true"
            android:launchMode="singleInstance"
            android:configChanges="layoutDirection|locale|grammaticalGender|fontScale|fontWeightAdjustment|orientation|uiMode|screenLayout|screenSize|smallestScreenSize|keyboard|keyboardHidden|navigation"
            android:preferMinimalPostProcessing="true"
            android:screenOrientation="fullSensor"
            android:exported="true">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
            <intent-filter>
                <action android:name="android.hardware.usb.action.USB_DEVICE_ATTACHED" />
            </intent-filter>
        </activity>
    </application>

</manifest>
